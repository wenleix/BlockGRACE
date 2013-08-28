/*
 *  ThreadTools.h
 *  GRACE
 *
 *  Created by Wenlei Xie on 5/7/12
 *  Updated by Wenlei Xie on 5/7/12
 *
 *  Copyright 2012 Cornell. All rights reserved.
 */

#ifndef _GRACE_THREAD_TOOLS_H_
#define _GRACE_THREAD_TOOLS_H_

#ifdef __APPLE__

#include <libkern/OSAtomic.h>
typedef OSSpinLock SpinLock;

inline void SpinLockInit(SpinLock *lock) {
    *lock = OS_SPINLOCK_INIT;
}

inline void SpinLockLock(SpinLock *lock) {
    OSSpinLockLock(lock);
}

inline void SpinLockUnlock(SpinLock *lock) {
    OSSpinLockUnlock(lock);
}

inline void SpinLockDestroy(SpinLock *lock) { }

#else

#include <pthread.h>
typedef pthread_spinlock_t SpinLock;

inline void SpinLockInit(SpinLock *lock) {
    pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE); 
}

inline void SpinLockLock(SpinLock *lock) {
    pthread_spin_lock(lock);
}

inline void SpinLockUnlock(SpinLock *lock) {
    pthread_spin_unlock(lock);
}

inline void SpinLockDestroy(SpinLock *lock) { 
    pthread_spin_destroy(lock);
}


#endif /* __APPLE__ */


#endif /* _GRACE_THREAD_TOOLS_H_ */
