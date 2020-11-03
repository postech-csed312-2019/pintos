#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>

/* === ADD START jinho q2-2 ===*/
#include "threads/synch.h"
/* === ADD END jinho q2-2 ===*/

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* === ADD START jinho p2q2 ===*/
#define FD_SIZE 256
#define FD_IDX_START 3
#define FD_STDIN_NUM 0
#define FD_STDOUT_NUM 1
#define FD_STDERR_NUM 2


/* === ADD END jinho p2q2 ===*/

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    /* === ADD START jihun q3 ===*/
    // NOTE : recent_cpu is saved as fixed point number
    int nice;
    int recent_cpu;
    /* === ADD END jihun ===*/
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* === ADD START jinho q1 ===*/
    int64_t wakeUpTick;
    struct list_elem sleep_elem;
    /* === ADD END jinho ===*/

    /* === ADD START jinho q2-2 ===*/
    // NOTE : original_priority value is designed only to be valid
    //        whenever it is being donated a priority
    int original_priority;
    struct lock* lock_acquiring;
    // NOTE : list donated_from stores the records of from which thread
    //        the thread got donated. donated_to_elem is used for storing
    // NOTE:  donated_to_elem should always be in a single donated_from
    //        list, since the thread can wait for at most 1 lock.
    struct list donated_from;
    struct list_elem donated_to_elem;
    /* === ADD END jinho q2-2 ===*/

    /* === ADD START jinho p2q2 ===*/
    tid_t ptid;                       /* Parent thread identifier. */

    struct list children;
    struct list_elem child_elem;
    struct semaphore child_sema;           /* Sema used when syscall handles exec() or wait() in a synchronized manner */

    bool init_done;
    bool init_status;                 /* true if success, else false */
    bool exit_done;
    int exit_status;
    bool exit_status_returned;
    /* === ADD END jinho p2q2 ===*/

    /* === ADD START jinho p2q2 ===*/
    struct file* fd_table[FD_SIZE];
    int fd_table_pointer;             /* where the last fd is stored ; initialized to 2 */
    /* === ADD END jinho p2q2 ===*/


#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);



/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);
/* === ADD START jinho q2-2 ===*/
void thread_set_priority_inner( struct thread* curThread, int new_priority,
        bool isPreemptRequired, bool isCalledByDonation );
/* === ADD END jinho q2-2 ===*/


/* === ADD START jinho q2-2 ===*/
void donate_priority(struct thread* cur, int priority);
void return_priority();
/* === ADD END jinho q2-2 ===*/

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

/* === ADD START jihun q3 ===*/
void thread_calculate_mlfqs_priority (struct thread *t);
void thread_calculate_recent_cpu (struct thread *t);
void thread_calculate_load_avg (void);
void thread_increment_recent_cpu(void);
void thread_recalculate_every_threads(void);
/* === ADD END jihun q3 ===*/

/* === ADD START jinho q1 ===*/
void thread_sleep(int64_t n);
void thread_awake();
bool compareThreadWakeUpTick(struct list_elem* e1, struct list_elem* e2, void* aux);
/* === ADD END jinho q1 ===*/

/* === ADD START jinho q2 ===*/
bool compareThreadPriority(struct list_elem* e1, struct list_elem* e2, void* aux);
/* === ADD END jinho q2 ===*/


#endif /* threads/thread.h */
