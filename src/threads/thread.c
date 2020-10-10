#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

/* === ADD START jinho q1 ===*/
#include "devices/timer.h"
/* === ADD END jinho q1 ===*/

/* === ADD START jihun q3 ===*/
#include "threads/fixed_point_arithmetic.h"
/* === ADD END jihun ===*/

#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* === ADD START jihun q3 ===*/
// NOTE : default values and variable for advanced scheduler
#define NICE_DEFAULT 0
#define RECENT_CPU_DEFAULT 0
#define LOAD_AVG_DEFAULT 0

int load_avg;
/* === ADD END jihun ===*/

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* === ADD START jinho q1 ===*/
static struct list sleep_list;
/* === ADD END jinho q1 ===*/

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame
{
  void *eip;                  /* Return address. */
  thread_func *function;      /* Function to call. */
  void *aux;                  /* Auxiliary data for function. */
};

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
                                        static void *alloc_frame (struct thread *, size_t size);
                                        static void schedule (void);
                                        void thread_schedule_tail (struct thread *prev);
                                        static tid_t allocate_tid (void);

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.
   Also initializes the run queue and the tid lock.
   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().
   It is not safe to call thread_current() until this function
   finishes. */
        void
        thread_init (void)
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);

  /* === ADD START jinho q1 ===*/
  list_init( &sleep_list );
  /* === ADD END jinho q1 ===*/

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();

}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void)
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  /* === ADD START jihun q3 ===*/
  // NOTE : initialize load_avg
  load_avg = LOAD_AVG_DEFAULT;
  /* === ADD END jihun ===*/

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void)
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
    else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}

/* Prints thread statistics. */
void
thread_print_stats (void)
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.
   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.
   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux)
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  /* Add to run queue. */
  thread_unblock (t);

/* === ADD START jinho q2 ===*/

  // NOTE : if cur_priority < new_thread_priority, new thread would already
  // have been at the front of queue. Thus we simply compare front/current
  // and yield conditionally.

  if( thread_current() != idle_thread) {
    bool createdThreadIsPrioritized = t->priority > thread_current()->priority;
    if( createdThreadIsPrioritized ){
      thread_yield();
    }
  }

/* === ADD END jinho q2 ===*/

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().
   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void)
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)
   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t)
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);

  /* === DEL START Jinho q2 === */

  //    list_push_back (&ready_list, &t->elem);
  /* === DEL END Jinho q2 === */

  /* === ADD START jinho q2 ===*/
  list_insert_ordered (&ready_list, &t->elem, &compareThreadPriority, NULL);
  /* === ADD END jinho q2 ===*/

  t->status = THREAD_READY;
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void)
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void)
{
  struct thread *t = running_thread ();

  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void)
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void)
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void)
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;

  ASSERT (!intr_context ());

  old_level = intr_disable ();
  /* === DEL START Jinho q2 === */

//    if (cur != idle_thread)
//        list_push_back (&ready_list, &cur->elem);
  /* === DEL END Jinho q2 === */

  /* === ADD START jinho q2 ===*/

  // case when thread is ready to run
  if (cur != idle_thread) {
    list_insert_ordered (&ready_list, &cur->elem, &compareThreadPriority, NULL);
  }
  /* === ADD END jinho q2 ===*/

  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* === ADD START jinho q1 ===*/

// NOTE : is run by user thread
void thread_sleep(int64_t ticks){
  enum intr_level old_level;
  int64_t curTime = timer_ticks();
  struct thread* curThread = thread_current();

  // handle invariants
  if( ticks <= 0 ) return;
  ASSERT( ticks > 0 );

  curThread->wakeUpTick = curTime + ticks;
  ASSERT( curThread->wakeUpTick - ticks == curTime );

  old_level = intr_disable ();
  if (curThread != idle_thread) {
    list_insert_ordered( &sleep_list, &(curThread->sleep_elem), &compareThreadWakeUpTick, NULL );
  }
  thread_block();
  intr_set_level (old_level);
}

// NOTE : is run by kernel thread when interrupt is raised.
void thread_awake() {
  int64_t curTime = timer_ticks();
  enum intr_level old_level;
  struct thread* targetThread;

  while( 1 ){
    //condition
    if( list_size(&sleep_list) == 0) { break; }
    targetThread = list_entry(list_front(&sleep_list), struct thread, sleep_elem);
    ASSERT( targetThread != NULL);
    if( !(targetThread->wakeUpTick <= curTime) ){ break; }

    // body
    old_level = intr_disable();
    targetThread = list_entry( list_pop_front(&sleep_list), struct thread, sleep_elem);
    ASSERT( targetThread != NULL);

    thread_unblock( targetThread );
    intr_set_level (old_level);
  }
}

bool compareThreadWakeUpTick(struct list_elem* e1, struct list_elem* e2, void* aux){
  struct thread *t1 = list_entry(e1, struct thread, sleep_elem);
  struct thread *t2 = list_entry(e2, struct thread, sleep_elem);
  ASSERT(t1 != NULL || t2 != NULL);

  // Designed to align small element at first.
  return t1->wakeUpTick < t2->wakeUpTick ;

}
/* === ADD END jinho q1 ===*/


/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
  {
    struct thread *t = list_entry (e, struct thread, allelem);
    func (t, aux);
  }
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority)
{
  /* === DEL START Jinho q2 === */
  // thread_current ()->priority = new_priority;
  /* === DEL END Jinho q2 === */

  /* === ADD START jinho q2 ( changes moved to thread_set_priority_inner() ) === */
//  // NOTE : this function is only called when it is on RUNNING state.
//  ASSERT( new_priority >= PRI_MIN && new_priority <= PRI_MAX );
//  ASSERT( thread_current()->status == THREAD_RUNNING );
//
//  thread_current ()->priority = new_priority;
//
//  // NOTE : Priority Preemption
//  if( list_size(&ready_list) > 0 ){
//    int highestPriority = list_entry(list_begin (&ready_list), struct thread, elem)->priority;
//    struct thread* curThread = thread_current();
//    bool modifiedThreadIsNotPrioritized = highestPriority > curThread->priority;
//
//    if( modifiedThreadIsNotPrioritized ) {
//      thread_yield();
//    }
//  }
  /* === ADD END jinho q2 ===*/

  /* === ADD START jihun q3 ===*/
  // NOTE : When advanced scheduler works, priority cannot be changed to the appointment.
  if (thread_mlfqs)
    return;
  /* === ADD END jihun q3 ===*/

  /* === ADD START jinho q2-2 === */
  //  // NOTE : this function is only called when it is on RUNNING state.
  ASSERT( new_priority >= PRI_MIN && new_priority <= PRI_MAX );
  ASSERT( thread_current()->status == THREAD_RUNNING );
  // isPreemptRequired=true, isCalledByDonation=false
  thread_set_priority_inner( thread_current(), new_priority, true, false);
  /* === ADD END jinho q2 ===*/
}

/* === ADD START jinho q2-2 ===*/
void thread_set_priority_inner (struct thread* t, int new_priority,
                                bool isPreemptRequired, bool isCalledByDonation ) {

  // NOTE : this function is the inner function of thread_set_priority(),
  //        designed to handle various priority settings.

  ASSERT(new_priority >= PRI_MIN && new_priority <= PRI_MAX);

  // NOTE : the set_priority operation can be lazy, that is,
  //        we set new_priority to original_priority, but not changing
  //        current priority. This should be considered since the attempt
  //        to change priority whenever thread is donated should be banned.
  bool isLazyOperation = (list_size(&t->donated_from) > 0)
                         && (t->priority > new_priority);

  if ( !isCalledByDonation && isLazyOperation ) {
    t->original_priority = new_priority;
  } else {
    t->priority = new_priority;
    // NOTE : if original priority is larger, if should be updated as well.
    if( t->original_priority > new_priority ){
      t->original_priority = new_priority;
    }
  }

  // NOTE : Priority Preemption
  if (isPreemptRequired && (list_size(&ready_list) > 0)) {
    int highestPriority = list_entry(list_begin(&ready_list),
    struct thread, elem)->priority;
    struct thread *cur = thread_current();
    bool modifiedThreadIsNotPrioritized = highestPriority > cur->priority;

    if (modifiedThreadIsNotPrioritized) {
      thread_yield();
    }
  }
}
/* === ADD END jinho q2-2 ===*/


/* === ADD START jinho q2 ===*/
bool compareThreadPriority(struct list_elem* e1, struct list_elem* e2, void* aux) {
  // NOTE : the list entry is searched only from ready_list
  struct thread *t1 = list_entry(e1, struct thread, elem);
  struct thread *t2 = list_entry(e2, struct thread, elem);
  ASSERT(t1 != NULL || t2 != NULL);

  // Designed to align large element at first.
  return t1->priority > t2->priority ;
}
/* === ADD END jinho q2 ===*/

/* Returns the current thread's priority. */
int
thread_get_priority (void)
{
  return thread_current ()->priority;
}

/* === ADD START jinho q2-2 ===*/
void donate_priority (struct thread* cur, int priorityToDonate) {

  // NOTE : 'while' statement below should run at least once.
  //        We set priority recursively alongside the acquire chain,
  //        in order to handle nested donation case.
  ASSERT( cur->lock_acquiring != NULL );

  while( cur->lock_acquiring != NULL ){
    ASSERT( cur->lock_acquiring->holder != NULL );
    cur = cur->lock_acquiring->holder;
    // isPreemptRequired=false, isCalledByDonation=true
    thread_set_priority_inner(cur, priorityToDonate, false, true);
  }
}

void return_priority (struct lock* lock) {

  struct thread* cur = thread_current();

  // NOTE : first, all elements(threads) that acquires the same lock as the front item
  //        of donated_from list are removed. This is because, only single donation with
  //        highest priority is effective in real. Thus in return stage, we ignore
  //        all meaningless donations.

  struct list_elem *e;
  struct list_elem *e1;
  for (e = list_begin (&cur->donated_from); e != list_end (&cur->donated_from); e = e1) {
    e1 = list_next (e);
    struct thread *t = list_entry (e, struct thread, donated_to_elem);
    if( t->lock_acquiring == lock) {
      list_remove(e);
    }
  }

  bool isSingleDonationCase =
          list_size( &(cur->donated_from) ) == 0;

  if( isSingleDonationCase ) {
    // isPreemptRequired=true, isCalledByDonation=true
    thread_set_priority_inner( cur, cur->original_priority, true, true );
  }
  else {
    int nextPriority = list_entry( list_front(&(cur->donated_from)),
    struct thread, donated_to_elem)->priority;
    ASSERT( cur->priority >= nextPriority );
    // isPreemptRequired=true, isCalledByDonation=true
    thread_set_priority_inner( cur, nextPriority, true, true );
  }

}
/* === ADD END jinho q2-2 ===*/

/* === ADD START jihun q3 ===*/
void thread_calculate_mlfqs_priority (struct thread *t)
{
  // NOTE : Calculates priority used in advanced scheduler.
  if (t == idle_thread)
    return;

  int mlfqs_priority = N_TO_FP(PRI_MAX);
    // priority = PRI_MAX
  mlfqs_priority = X_MINUS_Y( mlfqs_priority, X_OVER_N( N_TO_FP(t->recent_cpu), 400 ) );
    // priority = PRI_MAX - (recent_cpu / 4)
  mlfqs_priority = X_MINUS_Y( mlfqs_priority, X_TIMES_N( N_TO_FP(t->nice), 2 ) );
    // priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)

  t->priority = X_TO_INT_TN(mlfqs_priority);
}

void thread_calculate_recent_cpu (struct thread *t)
{ // TODO recent_cpu
  // NOTE : Calculates recent_cpu used in advanced scheduler.
  if (t == idle_thread)
    return;

  int new_recent_cpu = X_OVER_N( N_TO_FP(load_avg), 50 );
    // recent_cpu = 2*load_avg
  new_recent_cpu = X_OVER_Y( new_recent_cpu, X_PLUS_N(new_recent_cpu, 1) );
    // recent_cpu = (2*load_avg)/(2*load_avg + 1)
  new_recent_cpu = X_TIMES_Y( new_recent_cpu, X_OVER_N( N_TO_FP(t->recent_cpu), 100) );
    // recent_cpu = (2*load_avg)/(2*load_avg + 1) * recent_cpu
  new_recent_cpu = X_PLUS_N( new_recent_cpu, t->nice );
    // recent_cpu = (2*load_avg)/(2*load_avg + 1) * recent_cpu + nice

  t->recent_cpu = X_TO_INT_TN( X_TIMES_N(new_recent_cpu, 100) );
}

int thread_calculate_ready_threads (void)
{
  // NOTE : Calculates number of ready_threads used in thread_calculate_load_avg().
  int count = 0;
  struct list_elem *e;
  for (e = list_begin (&ready_list); e != list_end (&ready_list); e = list_next (e))
    count++;
  if(thread_current () != idle_thread)
    count++;
  return count;
}

void thread_calculate_load_avg (void)
{
  // NOTE : Calculates load_avg used in advanced scheduler.
  int old_load_avg = X_OVER_N( N_TO_FP(load_avg), 100 );
  int num1 = X_OVER_Y( N_TO_FP(59), N_TO_FP(60) );
  int new_load_avg = X_TIMES_Y(num1, old_load_avg);
    // load_avg = (59/60)*load_avg
  int num2 = X_OVER_Y( N_TO_FP(1), N_TO_FP(60) );
  new_load_avg = X_PLUS_Y( new_load_avg, X_TIMES_N(num2, thread_calculate_ready_threads()) );
    // load_avg = (59/60)*load_avg + (1/60)*ready_threads

  load_avg = X_TO_INT_TN( X_TIMES_N(new_load_avg, 100) );
  if (load_avg < 0)
    load_avg = 0;
}

void thread_increment_recent_cpu(void)
{
  // NOTE : Increment recent_cpu by 1.
  if (thread_current() == idle_thread)
    return;

  thread_current()->recent_cpu += 100;
}

void thread_recalculate_every_threads(void)
{
  // NOTE : Recalculate recent_cpu and priority of every threads and load_avg.
  struct list_elem *e;
  for (e = list_begin (&all_list); e != list_end (&all_list); e = list_next (e))
  {
    struct thread *t = list_entry (e, struct thread, allelem);
    thread_calculate_recent_cpu(t);
    thread_calculate_mlfqs_priority(t);
  }
  thread_calculate_load_avg();
}
/* === ADD END jihun ===*/


/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED)
{
  /* === ADD START jihun q3 ===*/
  thread_current()->nice = nice;
  /* === ADD END jihun ===*/
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void)
{
  /* === ADD START jihun q3 ===*/
  return thread_current()->nice;
  /* === ADD END jihun ===*/

  /* === DEL START jihun q3 ===*/
  //return 0;
  /* === DEL END jihun ===*/
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void)
{
  /* === ADD START jihun q3 ===*/
  return load_avg;
  /* === ADD END jihun ===*/

  /* === DEL START jihun q3 ===*/
  //return 0;
  /* === DEL END jihun ===*/
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void)
{
  /* === ADD START jihun q3 ===*/
  return thread_current()->recent_cpu;
  /* === ADD END jihun ===*/

  /* === DEL START jihun q3 ===*/
  //return 0;
  /* === DEL END jihun ===*/
}

/* Idle thread.  Executes when no other thread is ready to run.
   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED)
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;)
  {
    /* Let someone else run. */
    intr_disable ();
    thread_block ();

    /* Re-enable interrupts and wait for the next one.
       The `sti' instruction disables interrupts until the
       completion of the next instruction, so these two
       instructions are executed atomically.  This atomicity is
       important; otherwise, an interrupt could be handled
       between re-enabling interrupts and waiting for the next
       one to occur, wasting as much as one clock tick worth of
       time.
       See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
       7.11.1 "HLT Instruction". */
    asm volatile ("sti; hlt" : : : "memory");
  }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux)
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void)
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the current thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->magic = THREAD_MAGIC;

  /* === ADD START jinho q1===*/
  t->wakeUpTick = 0;
  /* === ADD END jinho q1 ===*/

  /* === ADD START jinho q2-2 ===*/
  t->original_priority = t->priority;
  t->lock_acquiring = NULL;
  list_init ( &(t->donated_from) );
  /* === ADD END jinho q2-2 ===*/

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);

  /* === ADD START jihun q3 ===*/
  // NOTE : initialize nice and recent_cpu
  t->nice = NICE_DEFAULT;
  t->recent_cpu = RECENT_CPU_DEFAULT;
  /* === ADD END jihun ===*/
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size)
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void)
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.
   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).
   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.
   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();

  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread)
  {
    ASSERT (prev != cur);
    palloc_free_page (prev);
  }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.
   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void)
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void)
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);