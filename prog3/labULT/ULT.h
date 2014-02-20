
#ifndef _ULT_H_
#define _ULT_H_
#include <ucontext.h>
#include <deque>
#include <map>

using namespace std;

typedef int Tid;
#define ULT_MAX_THREADS 1024
#define ULT_MIN_STACK 32768



typedef struct ThrdCtlBlk{
  Tid tid;
  bool rfgc; 
  ucontext_t context;
} ThrdCtlBlk;

static deque<ThrdCtlBlk*> ready_list;
static deque<ThrdCtlBlk*> zombie_list;
static map<Tid, void*> tcb_stacks;
// 1 = taken
// 0 = available
static bool __attribute__ ((unused)) taken_tids[ULT_MAX_THREADS] = {1};


static int __attribute__ ((unused)) current_tid = 0;
/*
 * Tids between 0 and ULT_MAX_THREADS-1 may
 * refer to specific threads and negative Tids
 * are used for error codes or control codes.
 * The first thread to run (before ULT_CreateThread
 * is called for the first time) must be Tid 0.
 */
static const Tid ULT_ANY = -1;
static const Tid ULT_SELF = -2;
static const Tid ULT_INVALID = -3;
static const Tid ULT_NONE = -4;
static const Tid ULT_NOMORE = -5;
static const Tid ULT_NOMEMORY = -6;
static const Tid ULT_FAILED = -7;

static inline int ULT_isOKRet(Tid ret){
  return (ret >= 0 ? 1 : 0);
}

Tid ULT_CreateThread(void (*fn)(void *), const void *parg);
Tid ULT_Yield(Tid tid);
Tid ULT_DestroyThread(Tid tid);


ThrdCtlBlk * findTcb(Tid wantTid);
Tid nextAvailableTid(void);
void init(void);

#endif

