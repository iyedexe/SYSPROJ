// exception.cc
//      Entry point into the Nachos kernel from user programs.
//      There are two kinds of things that can cause control to
//      transfer back to here from user code:
//
//      syscall -- The user code explicitly requests to call a procedure
//      in the Nachos kernel.  Right now, the only function we support is
//      "Halt".
//
//      exceptions -- The user code does something that the CPU can't handle.
//      For instance, accessing memory that doesn't exist, arithmetic errors,
//      etc.
//
//      Interrupts (which can also cause control to transfer from user
//      code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "../machine/synchconsole.h"
#include "userthread.h"

extern void SynchPutChar(const char cr);
extern SynchConsole *synchconsole;
//----------------------------------------------------------------------
// UpdatePC : Increments the Program Counter register in order to resume
// the user program immediately after the "syscall" instruction.
//----------------------------------------------------------------------
static void
UpdatePC ()
{
    int pc = machine->ReadRegister (PCReg);
    machine->WriteRegister (PrevPCReg, pc);
    pc = machine->ReadRegister (NextPCReg);
    machine->WriteRegister (PCReg, pc);
    pc += 4;
    machine->WriteRegister (NextPCReg, pc);
}

static void copyStringFromMachine(int from, char *to, unsigned int size){
  unsigned int i; // counter
  int cr; // character read
  int addr; //address of the character to read
  for(i=0;i<size;i++){
    addr = (i+(machine->ReadRegister(from))); // get the address of the char
    machine->ReadMem(addr, 1, &cr); // get the char
    to[i] = (char) cr; // concat it
  }
  to[i]='\0';
}

static void copyStringToMachine(int to, char *from, unsigned int size){
  unsigned int i; // counter
  int addr; //address of the character to read
  for(i=0;i<size;i++){
    addr = (i+(machine->ReadRegister(to))); // get the address of the char
    machine->WriteMem(addr, 1, from[i]); // get the char
  }
}
//----------------------------------------------------------------------
// ExceptionHandler
//      Entry point into the Nachos kernel.  Called when a user program
//      is executing, and either does a syscall, or generates an addressing
//      or arithmetic exception.
//
//      For system calls, the following is the calling convention:
//
//      system call code -- r2
//              arg1 -- r4
//              arg2 -- r5
//              arg3 -- r6
//              arg4 -- r7
//
//      The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//      "which" is the kind of exception.  The list of possible exceptions
//      are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler (ExceptionType which)
{
    int type = machine->ReadRegister (2);

    #ifndef CHANGED

      if ((which == SyscallException) && (type == SC_Halt))
        {
  	  DEBUG ('a', "Shutdown, initiated by user program.\n");
  	  interrupt->Halt ();
        }
      else
        {
  	  printf ("Unexpected user mode exception %d %d\n", which, type);
  	  ASSERT (FALSE);
        }

    #else //CHANGED
      if(which == SyscallException){
        switch(type){
          case SC_Halt:{
            DEBUG ('a', "Shutdown, initiated by user program.\n");
            while(currentThread->space->getThreadNumber() > 1){
          //    printf("still threads not free : %d \n", currentThread->space->getThreadNumber());
              currentThread->space->lockEndMain();
            }
        	  interrupt->Halt ();
            break;
          }
          case SC_PutChar:{
            const char carcater = (char) machine->ReadRegister(4);
            synchconsole->SynchPutChar(carcater);
            break;
          }
          case SC_SynchPutString:{
            char stg[MAX_STRING_SIZE];
            copyStringFromMachine(4, stg, MAX_STRING_SIZE);
            synchconsole->SynchPutString(stg);
            //delete stg;
            break;
          }
          case SC_SynchGetChar:{
            machine->WriteRegister(2,(int) synchconsole->SynchGetChar());
            break;
          }
          case SC_SynchGetString:{
            char *stg = new char[MAX_STRING_SIZE];
            synchconsole->SynchGetString(stg , machine->ReadRegister(5));
            copyStringToMachine(4, stg, machine->ReadRegister(5));
            //delete  stg;
            break;
          }
          case SC_SynchPutInt:{
            char *s= new char[11];
            snprintf(s, 10, "%d", machine->ReadRegister(4));
            synchconsole->SynchPutString(s);
            delete s;
            break;
          }
          case SC_SynchGetInt:{
            char *stg = new char[10];
            int i=0;
            synchconsole->SynchGetString(stg , 11);
            sscanf(stg, "%d",&i);
            machine->WriteMem(machine->ReadRegister(4), 4, i); // get the char
            delete stg;
            break;
          }
          case SC_UserThreadCreate:{
            //TODO verify if there's too many proccess in which case return -1
            int tid;
            tid = do_UserThreadCreate((int) machine->ReadRegister(4), (int) machine->ReadRegister(5));
            machine->WriteRegister(2,tid);
            break;

          }
          case SC_UserThreadExit:{
            do_UserThreadExit();
            break;
          }
          case SC_UserThreadJoin:{
            int tid = machine->ReadRegister(4);
            do_UserThreadJoin(tid);
            break;
          }
          case SC_Exit:{
            do_UserThreadExit();
            break;
          }
          default:{
            printf ("Unexpected user mode exception %d %d\n", which, type);
            ASSERT (FALSE);
          }
        }
      }
    #endif //CHANGED
    // LB: Do not forget to increment the pc before returning!
    UpdatePC ();
    // End of addition
}
