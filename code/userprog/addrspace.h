// addrspace.h
//      Data structures to keep track of executing user programs
//      (address spaces).
//
//      For now, we don't keep any information about address spaces.
//      The user level CPU state is saved and restored in the thread
//      executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "synch.h"
#include "bitmap.h"
//#include "frameprovider.h"

#define UserStackSize	2048	// increase this as necessary!
#define threadPages 2


class Semaphore;

class AddrSpace
{
  public:
    AddrSpace (OpenFile * executable);	// Create an address space,
    // initializing it with the program
    // stored in the file "executable"
    ~AddrSpace ();		// De-allocate an address space

    void InitRegisters ();	// Initialize user-level CPU registers,
    // before jumping to user code

    void SaveState ();		// Save/restore address space-specific
    void RestoreState ();	// info on a context switch

    int threadNumber;
    Semaphore *semThreadNumber;
    int getThreadNumber();
    void newUserThread();
    void deleteUserThread();
    int getNextThreadSpace();

    BitMap *threadBitMap;
    Semaphore *semBitMap;

    Semaphore *semEndMain;
    void lockEndMain();
    void freeEndMain();

    int GetTid();
    void RemoveTid();
    Semaphore* semThreadId;
    int tidCount;
    int MaxThreadNumber;
    Semaphore* semThreadJoin[10];

    int getSpaceAllocation();
    void setSpaceAllocation(int i);

//    FrameProvider *frameProviderProcs;

  private:
      TranslationEntry * pageTable;	// Assume linear page table translation
    // for now!
    unsigned int numPages;	// Number of pages in the virtual
    // address space
    int spaceAllocation;
};

#endif // ADDRSPACE_H
