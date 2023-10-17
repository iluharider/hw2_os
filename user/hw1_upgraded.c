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

  int lock = blocking(BL_GET, -1);                                         //INIT LOCK
  error_exiting(lock, "initializing of lock didn't work");

  int childPid = fork();
  error_exiting(childPid, "can't create child process");
  if (childPid == 0) {
	close(parent_pipe[0]); 
	close(child_pipe[1]);  

	error_exiting(blocking(BL_ACQUIRE, lock), "can't lock sleeplock");    //ACQUIRE LOCK 
	read_write(child_pipe[0], parent_pipe[1]);
	error_exiting(blocking(BL_RELEASE, lock), "can't release sleeplock"); //RELEASE LOCK 

  } else {
	close(parent_pipe[1]); 
	close(child_pipe[0]);

	for (int i = 0; argv[1][i] != '\0'; ++i)
	    write(child_pipe[1], argv[1] + i, 1);
	close(child_pipe[1]);

	error_exiting(blocking(BL_ACQUIRE, lock), "can't lock sleeplock");    //ACQUIRE LOCK 
	read_write(parent_pipe[0], -1);
	error_exiting(blocking(BL_RELEASE, lock), "can't release sleeplock"); //RELEASE LOCK 	

	close(parent_pipe[0]);
	error_exiting(blocking(BL_DELETE, lock), "can't delete sleeplock");     //FREE LOCK
  }
  return 0;
}