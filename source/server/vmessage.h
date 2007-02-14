/*
Copyright c1997-2007 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.7
http://www.bombaydigital.com/
*/

#ifndef vmessage_h
#define vmessage_h

#include "vtypes.h"
#include "vstring.h"
#include "vbinaryiostream.h"
#include "vmutex.h"
#include "vmemorystream.h"
#include "vlogger.h"
#include <deque>

/** @file */

/**
    @ingroup vsocket
*/

class VServer;

typedef Vs32 VMessageLength;    ///< The length of a message. Meaning and format on the wire are determined by actual message protocol.
typedef Vs16 VMessageID;        ///< Message identifier (verb) to distinguish it from other messages in the protocol.

class VMessagePool;

/**
VMessage is an abstract base class that implements the basic messaging
capabilities; the concrete subclass must implement the send() and receive()
functions, which know how to read and write the particular message protocol
format (the wire protocol). This class works with VMessagePool by allowing
messages to be "recycled"; recycling takes an existing pooled message and
makes it ready to be used again as if it had been newly instantiated.
*/
class VMessage : public VBinaryIOStream
	{
	public:
	
		// Constants for the recycle() parameter makeEmpty.
		static const bool kMakeEmpty = true;	///< (Default) The message buffer length will be set to zero, effectively resetting the message buffer to empty.
		static const bool kKeepData = false;	///< The message buffer will be left alone so that the existing message data can be retained.

		/**
		Constructs an empty message with no message ID defined,
		suitable for use with receive(). You can also set the
		message ID afterwards with setMessageID().
		*/
		VMessage(Vs64 initialBufferSize=1024);
		/**
		Constructs a message with a message ID, suitable for use
		with send(), optionally writing message data first.
		@param	messageID		the message ID
		*/
		VMessage(VMessageID messageID, VMessagePool* pool, Vs64 initialBufferSize=1024);
		/**
		Virtual destructor.
		*/
		virtual ~VMessage() {}
        
        /**
        Returns the pool to which this message belongs; when releasing
        a message, it must be released to the correct pool.
        @return the pool this message belongs to
        */
        VMessagePool* getPool() const { return mPool; }
		
		/**
		Re-initializes the message to be in a usable state as if
		it had just been instantiated; useful when re-using a single
		message object for multiple messages, or when pooling.
		@param	messageID	the message ID to set for the message
		@param	makeEmpty	normally true, which resets the message length to zero; if false,
							leaves the message buffer alone so the data there remains intact
							and available as the recycled message's data
		*/
		void recycle(VMessageID messageID=0, bool makeEmpty=kMakeEmpty);
		
		/**
		Sets the message ID, which is used when sending.
		@param	messageID	the message ID
		*/
		void setMessageID(VMessageID messageID) { mMessageID = messageID; }
		/**
		Returns the message ID.
		*/
		VMessageID getMessageID() const { return mMessageID; }
		
		/**
		Sends the message to the output stream, using the appropriate wire
		protocol message format; for example, it might write the message
		data content length, the message ID, and then the message data. The
		message data content is stored in the mMessageDataBuffer and it is
		typically just copied to the output stream using <code>streamCopy()</code>.
		The data length can be obtained by calling
		<code>this->getMessageDataLength()</code>.
		@param	sessionLabel	a label to use in log output, to identify the session
		@param	out				the stream to write to
		*/
		virtual void send(const VString& sessionLabel, VBinaryIOStream& out) = 0;
		/**
		Receives the message from the input stream, using the appropriate wire
		protocol format; for example, it might read the message data content
		length, the message ID, and then the message data. The message data
		should be read into the mMessageDataBuffer, typically by calling
		<code>streamCopy()</code> after you have read the length value.
		@param	sessionLabel	a label to use in log output, to identify the session
		@param	in				the stream to read from
		*/
		virtual void receive(const VString& sessionLabel, VBinaryIOStream& in) = 0;
		/**
		Copies this message's data to the target message's data buffer.
		The target's ID and other meta information (such as broadcast
		info) is not altered. This message's i/o offset is restored
		upon return, so its internal state is essentially untouched.
		The target message's offset is honored and altered, so you
		could use this function to append data to the target message
		at its current i/o offset.
		I've declared mMessageDataBuffer mutable so that this function
		can be properly declared const and still call non-const functions
		of mMessageDataBuffer (it saves and restores the buffer's state).
		*/
		void copyMessageData(VMessage& targetMessage) const;

		/**
		Returns the message data length (does not include the length of
		the message ID nor the message length indicator itself).
		@return the message data length
		*/
		VMessageLength getMessageDataLength() const;
		/**
		Returns a pointer to the raw message data buffer -- should only be used
		for debugging and logging purposes. The length of the valid data in the buffer
		is getMessageDataLength(). The returned pointer is only guaranteed to be
		valid as long as the message itself exists and is not written to (writing
		may require the buffer to be reallocated).
		@return a pointer to the raw message data buffer
		*/
		Vu8* getBuffer() const;
		/**
		Returns the total size of the memory buffer space consumed by this
		message; this is mainly for use in logging and debugging.
		@return the message data length
		*/
		Vs64 getBufferSize() const;
		/**
		Returns true if this message is being broadcast.
		@return true if this message is being broadcast
		*/
		bool isBeingBroadcast() const { return mIsBeingBroadcast; }
		/**
		Marks this message as being for broadcast. This must be called by any broadcast
		functions (i.e., bottlenecked in VServer::postBroadcastMessage) in
		order to mark the message so that during pool release we know to lock the
		broadcast mutex and correctly release the message only when the last broadcast
		target is done with the message.
		@return true if this message is being broadcast
		*/
		void markForBroadcast() { mIsBeingBroadcast = true; }
		/**
		Returns the number of outstanding broadcast targets; caller must lock mutex as appropriate.
		@return the number of outstanding broadcast targets
		*/
		int numBroadcastTargets() const { return mNumBroadcastTargets; }
		/**
		Returns a pointer to the broadcast mutex, which a caller can
		lock while adding the set of broadcast targets to the message.
		@return a pointer to the broadcast mutex
		*/
		VMutex* getBroadcastMutex() { return &mBroadcastMutex; }
		/**
		Increments this message's broadcast target count; caller must lock mutex as appropriate.
		*/
		void addBroadcastTarget();
		/**
		Decrements this message's broadcast target count; caller must lock mutex as appropriate.
		*/
		void removeBroadcastTarget();

		/*
		We use more granular log levels, so that the amount of log
		output we generate can be fine-tuned by the user.
		The message handlers emit a message at kDebug level when
		they are called.
		The message handlers emit a message at kDebug + 2 level
		with their nicely-displayed message data.
		The prettier but detailed message element logging is done
		by the message handlers at kDebug + 2 level.
		So:
		kDebug + 0 : the dispatched message handler logs its name/ID.
		kDebug + 1 : the message properties (ID,length) are logged before dispatch
		kDebug + 2 : the dispatched message handler logs the message fields in readable form
		kDebug + 3 : the message data is logged in hex before dispatch
		(Similar behavior for outbound messages.)

		Also, we trace some troubleshooting diagnostic items at level 100 (kAll).
		*/
		static const int kMessageReceiveSimpleLogLevel	= VLogger::kDebug + 1;
		static const int kMessageReceiveHexDumpLogLevel	= VLogger::kDebug + 3;
		static const int kMessageReceiveTraceLogLevel	= VLogger::kTrace;
		static const int kMessageSendSimpleLogLevel		= VLogger::kDebug + 1;
		static const int kMessagePostHexDumpLogLevel	= VLogger::kDebug + 3;
		static const int kMessageSendHexDumpLogLevel	= VLogger::kDebug + 4;
		static const int kMessagePoolTraceLogLevel		= VLogger::kTrace;
		
		static const VString kMessageLoggerName;

	protected:

		mutable VMemoryStream	mMessageDataBuffer;		///< The buffer that holds the message data. Mutable because copyMessageData needs to touch it and restore it.

	private:
	
		VMessageID				mMessageID;				///< The message ID, either read during receive or to be written during send.
        VMessagePool*           mPool;                  ///< The pool where this message should be released to.
		bool					mIsBeingBroadcast;		///< True if this message is an outbound broadcast message.
		int						mNumBroadcastTargets;	///< Number of pending broadcast targets, if for broadcast.
		VMutex					mBroadcastMutex;		///< Mutex to control multiple threads using this message during broadcasting. This is declared public because the caller is responsible for locking this mutex via a VMutexLocker while posting for broadcast.
	};

/**
VMessageFactory is an abstract base class that you must implement and
supply to a VMessagePool, so that the pool can instantiate new messages
when needed. All you have to do is implement the instantiateNewMessage()
function to return a new VMessage of the desired subclass type.
*/
class VMessageFactory
	{
	public:
	
		VMessageFactory() {}
		virtual ~VMessageFactory() {}

		/**
		Must be implemented by subclass, to simply instantiate a
		new VMessage object of a concrete VMessage subclass type.
		@param	messageID	the ID to supply to the message constructor
		@return	pointer to a new message object
		*/		
		virtual VMessage* instantiateNewMessage(VMessageID messageID, VMessagePool* pool) = 0;
	};

#endif /* vmessage_h */