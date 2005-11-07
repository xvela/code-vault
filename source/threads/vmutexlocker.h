/*
Copyright c1997-2005 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 2.3.2
http://www.bombaydigital.com/
*/

#ifndef vmutexlocker_h
#define vmutexlocker_h

/** @file */

#include "vtypes.h"

class VMutex;

/**
    @ingroup vthread
*/

/**
VMutexLocker is a helper class that you can use to make working with
mutexes easier, and more importantly to guarantee proper release behavior
of a VMutex even when exceptions are raised.

VMutexLocker lets you avoid ugly code to properly manage the mutex, and
instead let C++ auto object destruction do the work for you.

Typically, you need to acquire and release a mutex lock in a function in order
to guarantee thread-safety. It is important that the mutex lock gets released
when you are done with it. If you throw an exception, this is ugly to
properly code, but VMutexLocker makes it trivial.

<code>
void doSomethingSafelyToAnObject(MyObject obj)<br>
    {<br>
    VMutexLocker    locker(obj.mMutex);<br>
    <br>
    obj.somethingDangerous(); // might throw an exception!<br>
    <br>
    if (obj.trouble())<br>
    &nbsp;&nbsp;&nbsp;&nbsp;throw VException("Oh no!");<br>
    }
</code>

In the example above, you are guaranteed that the MyObject's mMutex will be
properly unlocked no matter whether you or something you call throws an
exception. This is because the locker object is guaranteed to be properly
destructed when the function scope exits, and the object's destructor
releases the mutex lock.

You can call the lock() method separately if you need to construct the
VMutexLocker without locking right away, but lock it later.

You can call the unlock() method separately if you need to unlock the
mutex before the VMutexLocker is destructed. Another useful technique to cause
the lock to be released earlier than the end of a function is to create a
scope specifically to surround the VMutexLocker's existence.

@see    VMutex
*/
class VMutexLocker
    {
    public:
    
        /**
        Constructs the locker, and if specified, acquires the mutex lock. If
        the mutex is already locked by another thread, this call blocks until
        it obtains the lock.
        
        You can pass NULL to constructor if you don't want anything to
        happen; this can useful if, for example, you allow a NULL VMutex
        pointer to be passed to a routine that needs to lock it if supplied.
        
        @param    inMutex            the VMutex to lock, or NULL if no action is wanted
        @param    lockInitially    true if the lock should be acquired on construction
        */
        VMutexLocker(VMutex* inMutex, bool lockInitially=true);
        /**
        Destructor, unlocks the mutex if this object has acquired it.
        */
        virtual ~VMutexLocker();
        
        /**
        Acquires the mutex lock; if the mutex is currently locked by another
        thread, this call blocks until the mutex lock can be acquired (if
        several threads are competing, the order in which they acquire the
        mutex is not known).
        */
        void    lock();
        /**
        Releases the mutex lock; if one or more other threads is waiting on
        the mutex, one of them will unblock and acquire the mutex lock once
        this thread releases it.
        */
        void    unlock();
        /**
        Returns true if this object has acquired the lock.
        @return    true if this object has acquired the lock
        */
        bool    isLocked() const { return mIsLocked; }
        /**
        Returns a pointer to the VMutex object.
        @return a pointer to the VMutex object (may be NULL)
        */
        VMutex*    mutex() { return mMutex; }
    
    protected:
    
        VMutex*    mMutex;        ///< Pointer to the VMutex object, or NULL.
        bool    mIsLocked;    ///< True if this object has acquired the lock.
    };

#endif /* vmutexlocker_h */

