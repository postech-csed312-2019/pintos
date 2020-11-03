#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/* === ADD START jinho p2q2 ===*/
#include <list.h>
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "devices/input.h"
/* === ADD END jinho p2q2 ===*/


static void syscall_handler (struct intr_frame *);

/* === ADD START jinho p2q2 ===*/

// NOTE : We write the function headers in the same file since
//        the functions MUST be accessed only from this file.

static void halt(void);
static void exit(int);
static pid_t exec(const char *);
static int wait(pid_t);
static bool create(const char *, unsigned);
static bool remove(const char *);
static int open(const char *);
static int filesize(int);
static int read(int, void *, unsigned);
static int write(int, const void *, unsigned);
static void seek(int, unsigned);
static unsigned tell(int);
static void close(int);

// NOTE : helper functions (locally used)
static bool isValidPointer(const void *);
static void handleInvalidUserPointer(const void *);

/* === ADD END jinho p2q2 ===*/

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  /* === ADD START jinho p2q2 ===*/
  lock_init(&fs_lock);
  /* === ADD END jinho p2q2 ===*/
}

/* === DEL START jinho p2q2 ===*/
//static void
//syscall_handler (struct intr_frame *f UNUSED)
//{
//  printf ("system call!\n");
//  thread_exit ();
    /* === DEL END jinho p2q2 ===*/


    /* === ADD START jinho p2q2 ===*/
static void
syscall_handler (struct intr_frame *f)
{
  const uint32_t* args[4];
  args[0] = f->esp;
  args[1] = (f->esp+1);
  args[2] = (f->esp+2);
  args[3] = (f->esp+3);

  handleInvalidUserPointer(args[0]);
  int syscall_number = *(args[0]);

  switch (syscall_number) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      handleInvalidUserPointer(args[1]);
      exit(args[1]);
      break;
    case SYS_EXEC:
      handleInvalidUserPointer(args[1]);
      f->eax = exec((char *) args[1]);
      break;
    case SYS_WAIT:
      handleInvalidUserPointer(args[1]);
      f->eax = wait(*(args[1]));
      break;
    case SYS_CREATE:
      handleInvalidUserPointer(args[1]);
      handleInvalidUserPointer(args[2]);
      f->eax = create((char *) *(args[1]), *(args[2]));
      break;
    case SYS_REMOVE:
      handleInvalidUserPointer(args[1]);
      f->eax = remove((char *) *(args[1]));
      break;
    case SYS_OPEN:
      handleInvalidUserPointer(args[1]);
      f->eax = open((char *) *(args[1]));
      break;
    case SYS_FILESIZE:
      handleInvalidUserPointer(args[1]);
      f->eax = filesize (*(args[1]));
      break;
    case SYS_READ:
      handleInvalidUserPointer(args[1]);
      handleInvalidUserPointer(args[2]);
      handleInvalidUserPointer(args[3]);
      f->eax = read(*(args[1]), (void *) *(args[2]), *(args[3]));
      break;
    case SYS_WRITE:
      handleInvalidUserPointer(args[1]);
      handleInvalidUserPointer(args[2]);
      handleInvalidUserPointer(args[3]);
      f->eax = write(*(args[1]), (void *) *(args[2]), *(args[3]));
      break;
    case SYS_SEEK:
      handleInvalidUserPointer(args[1]);
      handleInvalidUserPointer(args[2]);
      seek(*(args[1]), *(args[2]));
      break;
    case SYS_TELL:
      handleInvalidUserPointer(args[1]);
      f->eax = tell(*(args[1]));
      break;
    case SYS_CLOSE:
      handleInvalidUserPointer(args[1]);
      close(*(args[1]));
      break;
    default:
      // NOTE : invalid system call
      exit(-1);
  }
  return;
  /* === ADD END jinho p2q2 ===*/
}

/* === ADD START jinho p2q2 ===*/

// NOTE : Kernel function implementation.
void exit(int status) {
  struct thread* cur = thread_current();
  printf ("%s: exit(%d)\n", cur->name, status);
  cur->exit_status = status;
  thread_exit ();
}

void halt(void) {
  shutdown_power_off ();
}

pid_t exec(const char *cmd_line) {
  tid_t child_tid;
  struct thread* cur = thread_current();
  child_tid = process_execute(cmd_line);
  struct thread* child = getChildPointer(cur, child_tid);
  ASSERT( child != NULL);

  sema_down( child->child_sema );
  ASSERT( child->init_done == true );

  if( child->init_status == false ){
    return -1;
  }
  return (pid_t) child_tid;
}

int wait(pid_t pid) {
  tid_t child_tid = (tid_t) pid;
  struct thread* cur = thread_current();
  struct thread* child = getChildPointer(cur, child_tid);

  if( child == NULL ) { return -1; }
  if( child->exit_status_returned == true) { return -1; }

  // Only valid wait can proceed.
  ASSERT( child != NULL );
  return process_wait(pid);
}

bool create(const char *file_name, unsigned size){
  bool status;
  lock_acquire(&fs_lock);
  status = filesys_create(file_name, size);
  lock_release(&fs_lock);
  return status;
}

bool remove(const char *file_name){
  bool status;
  lock_acquire(&fs_lock);
  status = filesys_remove(file_name);
  lock_release(&fs_lock);
  return status;
}

int open(const char *file_name){

}

int filesize(int fd){

}

int read(int fd, void *buffer, unsigned size){

}

int write(int fd, const void *buffer, unsigned size){

}

void seek(int fd, unsigned position){

}

unsigned tell(int fd){

}

void close(int fd){

}

/* === ADD END jinho p2q2 ===*/

/* === ADD START jinho p2q2 ===*/

// NOTE : helper functions
static bool isValidUserPointer(const void *ptr) {
  // NOTE : check if ptr
  //        1. is not null
  //        2. references to user area
  //        3. references to a thread-owned area
  if( ptr==NULL ) { return false; }
  if( !is_user_vaddr(ptr) ) { return false; }
  struct thread *cur = thread_current();
  if( pagedir_get_page(cur->pagedir, ptr) == NULL ) { return false; }

  // All three cases passed.
  return true;
}

static void handleInvalidUserPointer(const void * ptr) {
  if( !isValidUserPointer(ptr) ){
    exit(-1);
  }
}

// NOTE : the function returns NULL if child not found.
struct thread* getChildPointer(struct thread* cur, tid_t child_tid){

  struct list* pChildList = &(cur->children);
  struct thread* out = NULL;

  struct list_elem *e;
  struct thread* thr;
  for (e = list_begin (pChildList); e != list_end (pChildList);
       e = list_next (e))
  {
    thr = list_entry (e, struct thread, list_elem);
    if( thr->tid == child_tid ){
      out = thr; break;
    }
  }
  return out;

}

/* === ADD END jinho p2q2 ===*/
