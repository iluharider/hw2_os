#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"

#include "blocking.h"

struct {
  struct spinlock sp_lock;
  struct sleeplock sl_locks[NUM_LOCKS];
  int is_locked[NUM_LOCKS];
} sleep_table;

void blockings_initialize() {
  initlock(&sleep_table.sp_lock, "sleep_table");

  for(int i = 0; i < NUM_LOCKS; ++i) {
	initsleeplock(&sleep_table.sl_locks[i], "sleep_table_lock");
  }
}

int blocking_get() {
  int result = -1;

  acquire(&sleep_table.sp_lock);
  for (int i = 0; i < NUM_LOCKS; ++i) {
	if (sleep_table.is_locked[i])
	  continue;
	result = i;
	sleep_table.is_locked[i] = 1;
	break;
  }

  release(&sleep_table.sp_lock);
  return result;
}

int valid_id(int id) {
  return id >= 0 && id < NUM_LOCKS && sleep_table.is_locked[id];
}

int blocking_block(int id) {
  if (!valid_id(id))
	return -1;
  acquiresleep(&sleep_table.sl_locks[id]);
  return 0;
}

int blocking_release(int id) {
  if (!valid_id(id))
	return -1;
  releasesleep(&sleep_table.sl_locks[id]);
  return 0;
}

int blocking_delete(int id) {
  if (!valid_id(id))
	return -1;
  sleep_table.is_locked[id] = 0;
  return 0;
}