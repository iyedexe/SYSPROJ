// addrspace.cc
//      Routines to manage address spaces (executing user programs).
//
//      In order to run a user program, you must:
//
//      1. link with the -N -T 0 option
//      2. run coff2noff to convert the object file to Nachos format
//              (Nachos object code format is essentially just a simpler
//              version of the UNIX executable object code format)
//      3. load the NOFF file into the Nachos file system
//              (if you haven't implemented the file system yet, you
//              don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include <stdio.h>
#include <strings.h>		/* for bzero */
#include "frameprovider.h"

//----------------------------------------------------------------------
// SwapHeader
//      Do little endian to big endian conversion on the bytes in the
//      object file header, in case the file was generated on a little
//      endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader * noffH)
{
    noffH->noffMagic = WordToHost (noffH->noffMagic);
    noffH->code.size = WordToHost (noffH->code.size);
    noffH->code.virtualAddr = WordToHost (noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost (noffH->code.inFileAddr);
    noffH->initData.size = WordToHost (noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost (noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost (noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost (noffH->uninitData.size);
    noffH->uninitData.virtualAddr =
	WordToHost (noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost (noffH->uninitData.inFileAddr);
}

static void
ReadAtVirtual( OpenFile *executable, int virtualaddr, int numBytes, int position, TranslationEntry *new_pageTable,
                    unsigned new_numPages)
{
    /*Save of the former page_table*/
    TranslationEntry* former_pageTable = machine->pageTable;
    machine->pageTable = new_pageTable;

    /*Save of the former numPages*/
    unsigned int former_numPages = machine->pageTableSize;
    machine->pageTableSize = new_numPages; //numPages = size

    char save[numBytes];
    /*Save inside the buffer*/
    int verif;

    if((verif = executable->ReadAt(save, numBytes, position) != numBytes))
    {
        fprintf(stderr, "%s", "Error when reading the memory\n");
    }

    /*Writing back into the memory*/
    for (int i = 0; i < numBytes; i++)
    {
        if(machine->WriteMem(virtualaddr + i, sizeof(char), save[i]) != TRUE) //sizeof(char) = 1
        {
          fprintf(stderr, "%s", "Error when Writing in the meory\n");
        }
    }

    machine->pageTable = former_pageTable;
    machine->pageTableSize = former_numPages;
}


//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//      Create an address space to run a user program.
//      Load the program from a file "executable", and set everything
//      up so that we can start executing user instructions.
//
//      Assumes that the object code file is in NOFF format.
//
//      First, set up the translation from program memory to physical
//      memory.  For now, this is really simple (1:1), since we are
//      only uniprogramming, and we have a single unsegmented page table
//
//      "executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace (OpenFile * executable)
{
    NoffHeader noffH;
    unsigned int i, size;


    executable->ReadAt ((char *) &noffH, sizeof (noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
	(WordToHost (noffH.noffMagic) == NOFFMAGIC))
	SwapHeader (&noffH);
    ASSERT (noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;	// we need to increase the size
    // to leave room for the stack
    numPages = divRoundUp (size, PageSize);
    size = numPages * PageSize;

    ASSERT (numPages <= NumPhysPages);	// check we're not trying
    // to run anything too big --
    // at least until we have
    // virtual memory

    DEBUG ('a', "Initializing address space, num pages %d, size %d\n",
	   numPages, size);

    pageTable = new TranslationEntry[numPages];
    //FrameProvider* frameprovider = new FrameProvider(numPages);
    int store[numPages];

    for(i = 0; i < numPages; i++)
    {
      if((store[i] = machine->frameProviderProcs->GetEmptyFrame()) == -1)
      {
        fprintf(stderr, "%s", "Not enough space for the process.\n");
        setSpaceAllocation(-1);
      }
    }

    for (i = 0; i < numPages; i++)
      {
	  pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	  pageTable[i].physicalPage = store[i];
	  pageTable[i].valid = TRUE;
	  pageTable[i].use = FALSE;
	  pageTable[i].dirty = FALSE;
	  pageTable[i].readOnly = FALSE;	// if the code segment was entirely on
	  // a separate page, we could set its
	  // pages to be read-only
      }

      tidCount =0; // ID threads used for user join and returned at creation
      threadNumber = 1; //Number of active threads
      semThreadNumber = new Semaphore("semThreadNumber",1); //for mutual exclusion on threadNumber
      semEndMain = new Semaphore("semMainEnd",0); // to lock the main if its sons hav not exited yet
      semThreadId = new Semaphore("semThreadId",1); // for mutual exclusion on TID handling

      int lengthBitMap = (int)(UserStackSize/(threadPages*PageSize)); // max number of threads
      //     printf("max threads : <%d>\n",lengthBitMap );
      threadBitMap = new BitMap(lengthBitMap); // Creates a bitmap with the max number of threads
      threadBitMap->Mark(0); // Marks the main thread stack as taken
      semBitMap = new Semaphore("semBitMap",1); //for stack allocation

      for(int ij=0;ij<lengthBitMap;ij++){ // semaphore table for join calls
        this->semThreadJoin[ij] = new Semaphore("semThreadJoin",0);
      }


// zero out the entire address space, to zero the unitialized data segment
// and the stack segment

//    bzero (machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0)
      {
	  DEBUG ('a', "Initializing code segment, at 0x%x, size %d\n",
		 noffH.code.virtualAddr, noffH.code.size);
        #ifndef CHANGED
	  executable->ReadAt (&(machine->mainMemory[noffH.code.virtualAddr]),
			      noffH.code.size, noffH.code.inFileAddr);
        #else
        ReadAtVirtual(executable, noffH.code.virtualAddr, noffH.code.size, noffH.code.inFileAddr, pageTable, numPages);
        #endif //CHANGED


      }
    if (noffH.initData.size > 0)
      {
	  DEBUG ('a', "Initializing data segment, at 0x%x, size %d\n",
		 noffH.initData.virtualAddr, noffH.initData.size);
     #ifndef CHANGED
	  executable->ReadAt (&
			      (machine->mainMemory
			       [noffH.initData.virtualAddr]),
			      noffH.initData.size, noffH.initData.inFileAddr);
      #else
        ReadAtVirtual(executable, noffH.initData.virtualAddr, noffH.initData.size, noffH.initData.inFileAddr, pageTable, numPages);
      #endif //CHANGED

      }



}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//      Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace ()
{
/*
  for (unsigned j = 0; j < this->numPages; j++) {
        frameprovider->ReleaseFrame(this->pageTable[j].physicalPage);
  }
  */// LB: Missing [] for delete
  // delete pageTable;
  // End of modification
  for (int i = 0; i < (int)numPages; i++){
   machine->frameProviderProcs->ReleaseFrame(pageTable[i].physicalPage);
  }
   delete [] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//      Set the initial values for the user-level register set.
//
//      We write these directly into the "machine" registers, so
//      that we can immediately jump to user code.  Note that these
//      will be saved/restored into the currentThread->userRegisters
//      when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters ()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister (i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister (PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister (NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister (StackReg, numPages * PageSize - 16);
    DEBUG ('a', "Initializing stack register to %d\n",
	   numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//      On a context switch, save any machine state, specific
//      to this address space, that needs saving.
//
//      For now, nothing!
//----------------------------------------------------------------------

void
AddrSpace::SaveState ()
{
  pageTable = machine->pageTable;
  numPages = machine->pageTableSize;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//      On a context switch, restore the machine state so that
//      this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void
AddrSpace::RestoreState ()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}


int
AddrSpace::getThreadNumber(){
  return threadNumber;
}

void
AddrSpace::newUserThread(){
  semThreadNumber->P();
  threadNumber++;
  semThreadNumber->V();
}

void
AddrSpace::deleteUserThread(){
  semThreadNumber->P();
  threadNumber--;
  semThreadNumber->V();
}

int
AddrSpace::getNextThreadSpace() {
//VERSION avec BitMap
  semBitMap->P();

  int baseAdress = (numPages*PageSize);
  int toReturn = 0;
  int find = threadBitMap->Find();
  if (find == -1){
    toReturn = -1;
  }
  else{
    toReturn = baseAdress - (PageSize*threadPages*(find));
  }
  semBitMap->V();

  return toReturn;
}


void
AddrSpace::lockEndMain(){
  semEndMain->P();}

void
AddrSpace::freeEndMain(){
  semEndMain->V();}

int
AddrSpace::GetTid(){
    semThreadId->P();
    tidCount++;
    semThreadId->V();
    return tidCount;
  }


int
AddrSpace::getSpaceAllocation(){return spaceAllocation;}

void
AddrSpace::setSpaceAllocation(int i){this->spaceAllocation = i;}

void
AddrSpace::RemoveTid(){
      semThreadId->P();
      tidCount--;
      semThreadId->V();
}
