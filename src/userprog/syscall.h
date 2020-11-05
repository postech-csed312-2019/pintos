#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* === ADD START jinho p2q2 ===*/
#include "threads/thread.h"
typedef int pid_t;
/* === ADD END jinho p2q2 ===*/

void syscall_init (void);

/* === ADD START jinho p2q2 ===*/
// NOTE : define lock to guarantee only one thread is accessing
//        filesystem at a time. Since pintos is a single-core system,
//        it is okay to maintain 1 lock to the filesystem.
struct lock fs_lock;

// NOTE : helper functions (globally used)
struct thread* getChildPointer(struct thread*, tid_t);

/* === ADD END jinho p2q2 ===*/


#endif /* userprog/syscall.h */
