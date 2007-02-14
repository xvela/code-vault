/*
Copyright c1997-2007 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.7
http://www.bombaydigital.com/
*/

#ifndef vmessagehandler_h
#define vmessagehandler_h

#include "vtypes.h"
#include "vstring.h"
#include "vmutex.h"
#include "vmutexlocker.h"
#include "vlogger.h"
#include "vmessage.h"
#include <map>

/** @file */

/**
    @ingroup vsocket
*/

class VMessagePool;
class VServer;
class VClientSession;
class VSocketThread;

class VMessageHandlerFactory;
typedef std::map<VMessageID,VMessageHandlerFactory*> VMessageHandlerFactoryMap;

/**
VMessageHandler is the abstract base class for objects that process inbound
messages from various client connections. A VMessageHandler is constructed with
a message to be processed, and the server, session, and thread in which
or on behalf of which the handler is being executed. The base class
constructor is supplied an optional mutex to lock, and it does so using a
VMutexLocker instance variable mLocker, which subclass can choose to unlock
as appropriate. If no lock is needed, the subclass can either pass NULL for
the mutex in the base class constructor, or unlock the locker in the subclass
constructor. The concrete subclass needs to implement
the processMessage() function and do its work there to process the mMessage.
Because the mutex lock is acquired in the constructor, the
implementation may rely on exclusive access to the data that is protected by
the lock. This also means that if the handler runs for a significant amount
of time, it may need to take care to not keep the lock too long. Whether or not
the handler decides to farm its long-running work out to a background task or
do it locally, long-running work will have to release the lock and reacquire
it periodically, or other threads will be blocked for an unacceptably long
time. And if appropriate, a handler or background task should try to release
the lock as soon as possible when it no longer needs it, or if it doesn't
really need it in the first place. 
*/
class VMessageHandler
	{
	public:

		/**
		Returns a message handler suitable for handling the specified
		message. When a message is read from the network, this is what
		is called to find a message handler for it. The message handler
		should then simply be told to processMessage(), and deleted.
		@param	m		the message to supply to the handler
		@param	server	the server to supply to the handler
		@param	session	the session for the client that sent this message, or NULL if n/a
		@param	thread	the thread processing the message (used to
						correlate the message to a client session)
		*/
		static VMessageHandler* get(VMessage* m, VServer* server, VClientSession* session, VSocketThread* thread);
		/**
		Registers a message handler factory for a particular
		message ID. When a call is made to get(), the appropriate
		factory function is called to create a handler for the message
		ID.
		*/
		static void registerHandlerFactory(VMessageID messageID, VMessageHandlerFactory* factory);
	
		/**
		Constructs a message handler with a message to handle and the
		server in which it is running.
		@param	m		the message to process
		@param	server	the server we're running in
		@param	session	the session for the client that sent this message, or NULL if n/a
		@param	thread	the thread processing the message (used to
						correlate the message to a client session)
		@param	pool	the message pool the
		                message is released to when the handler is cleaned up
        @param  mutex   if not null, a mutex that will be initially locked by the constructor
                        and unlocked by the destructor
		*/
		VMessageHandler(VMessage* m, VServer* server, VClientSession* session, VSocketThread* thread, VMessagePool* pool, VMutex* mutex);
		/**
		Virtual destructor. VMessageHandler normally does not delete the
		mMessage because the message will be put back in a message pool by the
		caller.
		*/
		virtual ~VMessageHandler();
		
		/**
		Processes the message.
		*/
		virtual void processMessage() = 0;
		/**
		Releases the message back to the pool. The subclass can prevent this
		if needs to hang onto the message for longer, by either setting
		fMessage to NULL before returning from processMessage() or by
		overridding this method.
		*/
		virtual void releaseMessage();
		/**
		Returns a message, which is either recycled from the pool, or newly
		instantiated if the pool is empty.
		@param	messageID		value with which to init the message's message ID
		@return a message object
		*/
		VMessage* getMessage(VMessageID messageID);
		/**
		Logs (at the appropriate log level) the supplied information about the
		message being handled. A message handler should call this to log the
		data contained in the inbound message, one element at a time. An
		optional facility here is that the caller may supplie the logger object
		to which the output will be written, and may obtain that object via
		a prior call to _getDetailsLogger(), which may return NULL. This allows
		the caller to: first call _getDetailsLogger() to obtain the logger
		object; if it's NULL (indicating the log level would emit nothing), it
		can avoid calling _logMessageDetails() at all and also avoid building
		the log message strings; and if the logger is not NULL (indicating the
		log level would emit data) then it can supply the logger to
		_logMessageDetails() so that this function doesn't have to keep re-finding
		the logger over repeated calls.
		@param	details	the text to be logged
		@param	logger	the logger to write to, or NULL to force the function to
						look up the logger
		*/
		void logMessageDetails(const VString& details, VLogger* logger=NULL) const;

	protected:
	
		/**
		Logs (at the appropriate log level) the message handler name to
		indicate that the handler has been dispatched to handle a message.
		@param	messageHandlerName	the handler name, which will be logged
		*/
		void _logSimpleDispatch(const VString& messageHandlerName) const;
		/**
		Returns a logger if message details should be logged. The message
		handler classes will call this before emitting the detailed log output,
		in order to be more efficient. If NULL is returned, the caller
		shouldn't log the message details; otherwise, the returned logger
		is the one it should log to, at kMessageDispatchDetailLogLevel level.
		@return	a logger, if message detail should be logged; NULL if not
		*/
		VLogger* _getDetailsLogger() const;
	
		static const int kMessageDispatchSimpleLogLevel = VLogger::kDebug;
		static const int kMessageDispatchDetailLogLevel = VLogger::kDebug + 2;
		static const int kMessageDispatchLifecycleLogLevel = VLogger::kTrace;

		VMessage*		mMessage;	///< The message this handler is to process.
		VServer*        mServer;	///< The server in which we are running.
		VClientSession* mSession;	///< The session for which we are running, or NULL if n/a.
		VSocketThread*	mThread;	///< The thread in which we are running.
        VMessagePool*   mPool;      ///< The pool to get/release messages from/to.
		VMutexLocker	mLocker;	///< The mutex locker for the mutex we were given.
		
	private:

		static VMessageHandlerFactoryMap* mapInstance();

		static VMessageHandlerFactoryMap* smFactoryMap;	///< The factories that create handlers for each ID.
	};

/**
VMessageHandlerFactory defines the interface for factory objects that know
how to create the appropriate type of concrete VMessageHandler subclass for
a particular message ID or set of message IDs. Normally you can just place
the DEFINE_MESSAGE_HANDLER_FACTORY macro in your message handler
implementation file.
*/
class VMessageHandlerFactory
	{
	public:
	
		VMessageHandlerFactory() {}
		virtual ~VMessageHandlerFactory() {}
		
		/**
		Instantiates a new message handler for the specified message's ID;
		must be overridden by the concrete subclass.
		@param	m		the message to be passed thru to the handler constructor
		@param	server	the serer to be passed thru to the handler constructor
		@param	session	the session to be passed thru to the handler constructor
		@param	thread	the thread to be passed thru to the handler constructor
        @param  mutex   the mutex to be passed thru to the handler constructor
		*/
		virtual VMessageHandler* createHandler(VMessage* m, VServer* server, VClientSession* session, VSocketThread* thread) = 0;
	};

// This macro goes in the handler's .h file to define the handler's factory.
#define DEFINE_MESSAGE_HANDLER_FACTORY(messageid, factoryclassname, handlerclassname) \
class factoryclassname : public VMessageHandlerFactory \
	{ \
	public: \
	\
		factoryclassname() : VMessageHandlerFactory() { VMessageHandler::registerHandlerFactory(messageid, this); } \
		virtual ~factoryclassname() {} \
		\
		virtual VMessageHandler* createHandler(VMessage* m, VServer* server, VClientSession* session, VSocketThread* thread) \
			{ return new handlerclassname(m, server, session, thread); } \
	\
	private: \
	\
		static factoryclassname smFactory; \
	\
	}

// This macro goes in the handler's .cpp file to declare the handler's factory.
#define DECLARE_MESSAGE_HANDLER_FACTORY(factoryclassname) \
factoryclassname factoryclassname::smFactory

// This macro goes in a global init function to prevent linker dead-stripping
// of the static initialization in the previous macro.
#define FORCE_LINK_MESSAGE_HANDLER_FACTORY(factoryclassname) \
if (false) { factoryclassname dummy; }

/**
This interface defines a background task object that can be attached to
a VClientSession, such that the session will not destruct until all attached
tasks have ended. If your VMessageHandler creates a task to perform work on
a separate thread, it should derive from VMessageHandlerTask so that the
session will know of its existence.
*/
class VMessageHandlerTask
    {
    public:
    
        VMessageHandlerTask() {}
        virtual ~VMessageHandlerTask() {}
    };

#endif /* vmessagehandler_h */