//
// Created by Wenxin Zheng on 2021/4/21.
//

#ifndef ACMOS_SPR21_ANSWER_LOCKS_H
#define ACMOS_SPR21_ANSWER_LOCKS_H


int lock_init(struct lock *lock){
    /* Your code here */
    if(nlock >= MAXLOCKS) BUG("Max lock count reached.");
    locks[nlock++] = lock;
    lock->locked=0;
	lock->cpuid=-1;
    return 0;
}

void acquire(struct lock *lock){
    /* Your code here */
    if(holding_lock(lock))
        BUG("Already acquired the lock!");
    while (try_acquire(lock)!=0)
    {
    }
    sync_synchronize();
	lock->cpuid=cpuid();
}

// Try to acquire the lock once
// Return 0 if succeed, -1 if failed.
int try_acquire(struct lock *lock){
    /* Your code here */
    if(holding_lock(lock))
        BUG("Already acquired the lock!");
    int id=__sync_lock_test_and_set(&(lock->locked),1);
    if(id!=0)
        return -1;
    sync_synchronize();
    __sync_lock_test_and_set(&(lock->cpuid),cpuid());
    return 0;
}

void release(struct lock* lock){
    if(cpuid()!=lock->cpuid)
        BUG("Try to release other's lock!");
    if(!is_locked(lock))
        BUG("Try to release an already unlocked lock!");
    lock->cpuid=-1;
    sync_synchronize();
    __sync_lock_release(&(lock->locked));
	
    /*int id=__sync_lock_test_and_set(&(lock->locked),0);
    if(id!=1)
        BUG("Try to release an already unlocked lock!");*/
    /* Your code here */
}

int is_locked(struct lock* lock){
    return lock->locked;
}

// private for spin lock
int holding_lock(struct lock* lock){
    /* Your code here */
    return lock->locked&&cpuid()==lock->cpuid;
}

#endif  // ACMOS_SPR21_ANSWER_LOCKS_H

