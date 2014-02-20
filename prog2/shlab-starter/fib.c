#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const int MAX = 13;

static void doFib(int n, int doPrint);



/*
 * unix_error - unix-style error routine.
 */
inline static void 
unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


int main(int argc, char **argv)
{
  int arg;
  int print;
 

  if(argc != 2){
    fprintf(stderr, "Usage: fib <num>\n");
    exit(-1);
  }

  if(argc >= 3){
    print = 1;
  }

  arg = atoi(argv[1]);
  if(arg < 0 || arg > MAX){
    fprintf(stderr, "number must be between 0 and %d\n", MAX);
    exit(-1);
  }

  doFib(arg, 1);

  return 0;
}

/* 
 * Recursively compute the specified number. If print is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once
 */
static void doFib(int n, int doPrint)
{ 
  int pid1 = 0, pid2 = 0;
  int status1, status2;
  int test1, test2;
  
  if(n == 0)
    exit(0);
  else if (n == 1 || n == 2)
    exit(1);
  else{
    if((pid1=fork()) == 0){
      doFib(n-1, 0);
      exit(n-1);
    }
    if((pid2=fork()) == 0){
      doFib(n-2, 0);
      exit(n-2);
    }
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    test1 = WEXITSTATUS(status1);
    test2 = WEXITSTATUS(status2);
    }
  printf("%d\n", test1);
  printf("%d\n", test2);
}

