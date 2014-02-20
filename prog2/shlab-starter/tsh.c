/* 
 * tsh - A tiny shell program with job control
 * 
 * Nelson Hazelbaker   20013552
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
extern int verbose;                 /* if true, print additional output */

extern char **environ;             /* defined in libc */
static char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
struct job_t jobs[MAXJOBS];       /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);


/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
    sleep(0);
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
	char *argv[MAXARGS]; 			/* arguments from command line */
	
	int isbg;	  		        /* is job bg or fg? */
	pid_t pid;		  	   	/* process ID */
	
	isbg = parseline(cmdline, argv); 	/* bg is 1 if background job, 0 if job is fg */
	
	// initialize SIGCHLD signals
	sigset_t sigset;  
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);

	if(!builtin_cmd(argv)) {/* this line executes built-in commands; if-statements are executed if not a built-in command*/
	sigprocmask(SIG_BLOCK, &sigset, NULL); 	/* block SIGCHLD signal */	
		if ((pid = fork()) == 0) { 				/* Child runs user job */
		// create new process group with child
			setpgid(0, 0);                 
			sigprocmask(SIG_UNBLOCK, &sigset, NULL); //unblock SIGCHLD signal
			
			if (execve(argv[0], argv, environ) < 0) { // execute path as command
				printf("%s: Command not found\n", argv[0]);
				fflush(stdout);
				exit(0);
			}
			exit(0);
		}
		if(!isbg){
			addjob(jobs, pid, FG, cmdline); 	        //add fg job
			sigprocmask(SIG_UNBLOCK, &sigset, NULL);	// unblock SIGCHLD signal			
			waitfg(pid); 					/* Parent waits for fg job to terminate */
		}
		else{
			addjob(jobs, pid, BG, cmdline); 	       //add bg job
			printf("[%d] (%d) %s",getjobpid(jobs, pid)->jid,pid,cmdline);  //print job details	
			sigprocmask(SIG_UNBLOCK, &sigset, NULL);    //unblock signal
		}
	}
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
    if(!strcmp(argv[0],"jobs")) 
    {
        listjobs(jobs);
        return 1;
    }

    if(!strcmp(argv[0],"bg") || !strcmp(argv[0],"fg"))
    {
        do_bgfg(argv);
	return 1;
    }

    if(!strcmp(argv[0],"&")) return 1;
    
    if(!strcmp(argv[0],"quit")) exit(0);

    return 0;     /* not a builtin command */
}
/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
     struct job_t* jobpro=NULL;
     char *jobid;
     int i;
     if(argv[1] == NULL) {  	        //no argument in command
        printf("%s Command requires PID or %%jobid\n", argv[0]);  
        return;  
    }
  	
    if(argv[1][0]=='%'){ 			
	jobid=argv[1]+sizeof(char); 
	for(i=0;i<strlen(jobid);i++){
	     if(!isdigit(jobid[i])) {
	        printf("%s: argument requires a PID %%jobid\n",argv[0]);	
	     return;
         }
    }	
    int jid=atoi(jobid);
		
    if((jobpro = getjobjid(jobs, jid)) == NULL) {   // check if job is in list
        printf("%%%s: No such job\n", jobid);  
        return;  
    }  		
	jobpro=getjobjid(jobs,jid);	// get job from jid	
	}
	else if(atoi(argv[1])!=0){     //pid argument
		jobid=argv[1];
	
		for(i=0;i<strlen(jobid);i++){
			if(!isdigit(jobid[i])) {
				printf("%s: argument requires a PID or %%jobid\n",argv[0]);
				return;
			}
		}			
		pid_t pid=atoi(jobid);
		if((jobpro=getjobpid(jobs, pid)) == NULL) {  		//check if process is in list
                   printf("(%s): No such process\n", jobid);  
                   return;  
                }  		
	}
	else {         //argument not jid or pid
		printf("%s: argument requires a PID or %%jobid\n",argv[0]);
		return;
	}
	if(!strcmp(argv[0],"bg")) 
	{
		jobpro->state=BG;   //set process to bg	
		kill(-(jobpro->pid), SIGCONT);   //continue process
		printf("[%d] (%d) %s\n", jobpro->jid,jobpro->pid, jobpro->cmdline);	
	}
	else
	{
		jobpro->state=FG; //move process to a fg process	
		kill(-(jobpro->pid), SIGCONT); //continue process
	
		waitfg(jobpro->pid); //wait for process to terminate
	}
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    int i;
    struct job_t* job; 
      
    while ((job =getjobpid(jobs,pid)) != NULL && (job->state==FG)){
         for(i = 0; i < 4; i++)
            sleep(0);
  
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    
    int pid=0;
    int status=-1;
    do{                                          //reap children, then delete from job list
        pid=waitpid(-1,&status,WNOHANG|WUNTRACED);   
        if(pid>0)
        {
            if(WIFEXITED(status)){               //child terminated, delete from job list
                deletejob(jobs,pid);
            }
            else
            if(WIFSIGNALED(status)){             //child terminated by another process
                if(WTERMSIG(status)==2)
                    printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(jobs, pid), pid, WTERMSIG(status));
                deletejob(jobs,pid);
            }
            else if(WIFSTOPPED(status)){         //child stopped by another process
                getjobpid(jobs, pid)->state=ST;
                printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(jobs, pid), pid, WSTOPSIG(status));
            }
        }        
    }
    while(pid>0);
    
}
/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig)
{       
    //search job list for fg process   
    int i=0;
    struct job_t* job=NULL;
    for(i=0;i<MAXJOBS;i++){        
        if(getjobjid(jobs,i)->state==FG && getjobjid(jobs,i)!=NULL)
            job=getjobjid(jobs,i);     
    }
    //send FG process a kill signal
    
    if(job != NULL){   
        kill(-(job->pid),SIGINT);              
    }
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig)
{    
     //search job list for fg process
    int i=0;
    struct job_t* job = NULL;
    for(i=0;i<MAXJOBS;i++){        
        if(getjobjid(jobs,i)->state==FG && getjobjid(jobs,i)!=NULL)
            job = getjobjid(jobs,i);
    }

    //send FG process a stop signal    
    if(job != NULL){ 
        kill(-job->pid,SIGTSTP);                   
    }   
    return;
}
/*********************
 * End signal handlers
 *********************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



