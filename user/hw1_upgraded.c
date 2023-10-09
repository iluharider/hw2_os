#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/blocking.h"

void error_exiting(int code, char* message) {
	if (code == -1) {
		printf("%d: %s\n", getpid(), message);
		exit(1);
	}
}

void read_write(int read_from, int write_to)
{
  char a[1];
  while (read(read_from, a, 1) > 0) {
	printf("%d: received %c\n", getpid(), a[0]);
	if (write_to != -1)
	  write(write_to, a, 1);
  }
  close(read_from);
  close(write_to);
}

int main(int argc, char* argv[])
{
  error_exiting((argc < 2 ? -1 : 0), "too few arguments");
  int child_pipe[2];
  int parent_pipe[2];
  error_exiting(pipe(child_pipe), "can't create pipe");
  error_exiting(pipe(parent_pipe), "can't create pipe");

  int childPid = fork();
  error_exiting(childPid, "can't create child process");
  if (childPid == 0) {
	close(parent_pipe[0]); 
	close(child_pipe[1]);  
	int lock;
	error_exiting(read(child_pipe[0], &lock, sizeof(lock)) > 0 ? 0 : -1, "can't read lock id from child");

	error_exiting(blocking(BL_ACQUIRE, lock), "can't lock sleeplock");
	read_write(child_pipe[0], parent_pipe[1]);
	error_exiting(blocking(BL_RELEASE, lock), "can't release sleeplock");
	error_exiting(blocking(BL_DELETE, lock), "can't delete sleeplock");

  } else {
	close(parent_pipe[1]); 
	close(child_pipe[0]);

	int lock = blocking(BL_GET, -1);
	error_exiting(lock, "can't create new sleep lock");
	error_exiting(blocking(BL_ACQUIRE, lock), "can't lock sleeplock");

	
	write(child_pipe[1], &lock, sizeof(lock));

	for (int i = 0; argv[1][i] != '\0'; ++i)
	    write(child_pipe[1], argv[1] + i, 1);
	close(child_pipe[1]);
	
	error_exiting(blocking(BL_RELEASE, lock), "can't release sleeplock");

	char a[1];
	int is_locked = 0;

	while (read(parent_pipe[0], a, 1) > 0) {
	  if (!is_locked) {
		error_exiting(blocking(BL_ACQUIRE, lock), "can't lock sleeplock");
		is_locked = 1;
	  }
	  printf("%d: received %c\n", getpid(), a[0]);
	}
	close(parent_pipe[0]);
  }
  return 0;
}