#ifndef HEADER_FILESYS_H
#define HEADER_FILESYS_H 1

#include <stdbool.h>
#include <stdint.h>
#include "off_t.h"

struct disk *filesys_disk;

struct file;
void filesys_init (bool format);
bool filesys_create (const char *name, off_t initial_size);
bool filesys_open (const char *name, struct file *);
bool filesys_remove (const char *name);
void filesys_list (void);
void filesys_print (void);

void filesys_self_test (void);

#endif /* filesys.h */
