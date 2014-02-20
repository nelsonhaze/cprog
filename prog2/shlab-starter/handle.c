#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"



/* Nelson Hazelbaker


 * First, print out the process ID of this process.
 *
 * Then, set up the signal handler so that ^C causes
 * the program to print "Nice try.\n" and continue looping.
 *
 * Finally, loop forever, printing "Still here\n" once every
 * second.
 */
void mysighandler(int signum)
{ ssize_t bytes;

  const int STDOUT = 1;
  
  if(signum == SIGUSR1){
      bytes = write(STDOUT, "exiting\n",8);
      if(bytes != 10)
     	 exit(-999);  
  }
  else{
      bytes = write(STDOUT, "Nice Try\n", 10);
      if(bytes != 10)
     	 exit(-999);
   }  
}


int main(int argc, char **argv)
{
  struct timespec req, rem;
  req.tv_sec = 1;
  req.tv_nsec = 0;

  signal(SIGINT, mysighandler);
  signal(SIGUSR1, mysighandler);

  printf("%d\n",getpid());

  while(1){
     if (nanosleep(&req, &rem) == -1) {
       continue;
     }
     else {
       printf("Still here\n");
     }
  }
  return 0;
}


