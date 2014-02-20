#include <cassert>
#include <cstdlib>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>

#include "ULT.h"


void stub() {
  Tid ret;
  
  /* call function */
  __asm__ (
   "mov    %esi,(%esp);" // move parg onto stack
   "call   %edi;"        // call function on parg
      );

  ret = ULT_DestroyThread(ULT_SELF);
  assert(ret == ULT_NONE);
  exit(0);
}

Tid ULT_CreateThread(void (*fn)(void *), const void *parg)
{
  Tid new_tid = nextAvailableTid();
  if(new_tid == ULT_NOMORE) {
    return ULT_NOMORE;
  }
  void * new_stack = malloc(ULT_MIN_STACK);
  if(!new_stack) {
    return ULT_NOMEMORY;
  }
  tcb_stacks[new_tid] = new_stack;
  ThrdCtlBlk * new_thread = (ThrdCtlBlk*)malloc(sizeof(ThrdCtlBlk));
  new_thread->rfgc = false;
  new_thread->tid  = new_tid;
  getcontext(&(new_thread->context));

//  new_thread->context.uc_stack.ss_size = ULT_MIN_STACK;
  new_thread->context.uc_mcontext.gregs[REG_ESP] = (unsigned int)new_stack + ULT_MIN_STACK - 4;
  new_thread->context.uc_mcontext.gregs[REG_EDI] = (unsigned int)fn;
  new_thread->context.uc_mcontext.gregs[REG_ESI] = (unsigned int)parg;
  new_thread->context.uc_mcontext.gregs[REG_EIP] = (unsigned int)&stub;
  
  ready_list.push_back(new_thread);
  
  return new_thread->tid;
}



Tid ULT_Yield(Tid wantTid)
{
  volatile Tid ret;
  if(wantTid < -2 || wantTid >= ULT_MAX_THREADS) {
    return ULT_INVALID;
  }
  // store current thread
  ThrdCtlBlk * current_thread = (ThrdCtlBlk*)malloc(sizeof(ThrdCtlBlk));
  current_thread->rfgc = false;
  current_thread->tid  = current_tid;
  getcontext(&(current_thread->context));
  if(!current_thread->rfgc) {
    current_thread->rfgc = true;
    ready_list.push_back(current_thread);

    // pick the next tcb to run 
    ThrdCtlBlk * tcb_to_run;
    if(wantTid == ULT_ANY) {
      tcb_to_run = ready_list.front();
      ready_list.pop_front();
    }
    else if(wantTid == ULT_SELF) {
      tcb_to_run = ready_list.back();
      ready_list.pop_back();
    } 
    else {
      tcb_to_run = findTcb(wantTid);
      if(!tcb_to_run) {
        ready_list.pop_back();
        return ULT_INVALID;
      }
    }
    current_tid = tcb_to_run->tid;
    ret = current_tid;
    setcontext(&(tcb_to_run->context));
  }

  if(ready_list.empty()) {
    if(wantTid == ULT_ANY) {
      ret = ULT_NONE;
    }
  }
  free(current_thread);
  return ret; 

}


Tid ULT_DestroyThread(Tid wantTid)
{
  if(wantTid < -2 || wantTid >= ULT_MAX_THREADS) {
    return ULT_INVALID;
  }
  ThrdCtlBlk * tcb_to_zombie;
  ThrdCtlBlk * tcb_to_run;

  deque<ThrdCtlBlk*>::iterator iter = zombie_list.begin();
  while(iter != zombie_list.end()) {
    Tid tid = (*iter)->tid;
    tcb_stacks.erase(tid);
    iter++;
  }
  zombie_list.resize(0);

  if(wantTid == ULT_ANY) {
    if(ready_list.empty()) {
      return ULT_NONE;
    }
    wantTid = ready_list.front()->tid;
  }

  if(wantTid == current_tid || wantTid == ULT_SELF) {
    ThrdCtlBlk * current_thread = (ThrdCtlBlk*)malloc(sizeof(ThrdCtlBlk));
    current_thread->tid  = current_tid;
    zombie_list.push_back(current_thread);
    taken_tids[current_tid] = 0;

    if(ready_list.empty()) {
      exit(0);
    }
    tcb_to_run = ready_list.front();
    ready_list.pop_front();
    current_tid = tcb_to_run->tid;
    setcontext(&(tcb_to_run->context));
  }
  // ready list cannot be self
  else {
    tcb_to_zombie = findTcb(wantTid);
    if(!tcb_to_zombie) {
      return ULT_INVALID;
    }
    // find removes from ready list
    taken_tids[tcb_to_zombie->tid] = 0;
    zombie_list.push_back(tcb_to_zombie);
  }

  return wantTid;
}

ThrdCtlBlk * findTcb(Tid wantTid) {
  ThrdCtlBlk * tcb_to_run;
  deque<ThrdCtlBlk*>::iterator iter = ready_list.begin();
  bool found_tcb = false;
  while((!found_tcb) && (iter != ready_list.end())) {
    if((*iter)->tid == wantTid) {
      tcb_to_run = *iter;
      ready_list.erase(iter);
      found_tcb = true;
    }
    iter++;
  }
  if(!found_tcb) {
    return (ThrdCtlBlk*)0;
  }
  return tcb_to_run;
}

Tid nextAvailableTid(void) {
  for(int ii = 0; ii < ULT_MAX_THREADS; ++ii) {
    if(!taken_tids[ii]) {
      taken_tids[ii] = 1;
      return ii;
    }
  }
  return ULT_NOMORE;
}

