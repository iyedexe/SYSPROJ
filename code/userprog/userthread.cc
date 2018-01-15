#include "userthread.h"
#include "machine.h"
#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "thread.h"
#include "syscall.h"
#include <stdio.h>

extern Machine *machine;
extern Thread *currentThread;

static void StartUserThread(int f){

  forkArgs *farg = (forkArgs *)f;

   currentThread->space->InitRegisters();
   currentThread->space->RestoreState();

   machine->WriteRegister (PCReg, farg->f);
   machine->WriteRegister (NextPCReg, farg->f + 4);
   machine->WriteRegister(4, farg->arg);
   int stackPointer = currentThread->space->getNextThreadSpace();
   ASSERT(stackPointer != -1)
   machine->WriteRegister (StackReg, stackPointer);
   //printf("arguments VS passed : %d , %d\n",machine->ReadRegister(4), farg->arg );
//   printf("Thread ID: %d \n", currentThread->getId() );


   machine->Run ();		// jump to the user progam

}


int do_UserThreadCreate(int f, int arg){

  //the current kernel thread must create a new thread newThread
  Thread *newThread = new Thread ("new Thread");
  forkArgs *farg = new forkArgs;

  farg->f = f;
  farg->arg = arg;
//  printf("arguments passed : %d , %d\n",f, arg );
  newThread->space = currentThread->space;
  newThread->setId(currentThread->space->GetTid());
  currentThread->space->newUserThread();

  //printf("thread number  : %d \n",currentThread->space->getThreadNumber() );
  //initialize it and place it in the threads queue
  newThread->Fork(StartUserThread,(int)farg);

  return newThread->getId();

}

void do_UserThreadExit(){
  //printf("new thread out, number pf threads : %d\n", currentThread->space->liveThreads);

  //un thread de moins
  currentThread->space->deleteUserThread();
  //printf("id thread : <%d>" ,currentThread->getId());
  currentThread->space->threadBitMap->Clear(currentThread->getId());
  currentThread->space->freeEndMain();
  currentThread->space->RemoveTid();

  currentThread->space->semThreadJoin[currentThread->getId()]->V();

  currentThread->Finish();
  //}

}

int do_UserThreadJoin(int tid){
  //printf("new thread out, number pf threads : %d\n", currentThread->space->liveThreads);
  //verify if thread exists
  /*if ((tid) > currentThread->space->getThreadNumber()){
    return -1;
  }*/

  //verify if pid different than current thread or main
  if(tid == currentThread->getId() || tid == 0){
    return -1;
  }

  //un thread de moins
  currentThread->space->semThreadJoin[tid]->P();
  return 0;
}
