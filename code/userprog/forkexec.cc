#include "forkexec.h"


void StartForkedProcess(int arg) {
    currentThread->space->RestoreState();
    currentThread->space->InitRegisters();
    //currentThread->space->InitMainThread();
    machine->Run();
}

int do_ForkExec (char *filename)
{
    AddrSpace *process;
    OpenFile *file;
    
    //File
    if ((file = fileSystem->Open(filename)) == NULL) 
    {
        fprintf(stderr, "%s", "Error when opening the file\n");
        return -1;
    }

    //Process
    if((process = new AddrSpace (file)) == NULL)
    {
        fprintf(stderr, "%s", "Error when allocating the memory for the process\n");
        return -1;
    }

    if(process->getSpaceAllocation() == -1)
    {
        fprintf(stderr, "%s", "Not enough space for the process\n");
        return -1;
    }

    Thread* thread = new Thread(filename);
    machine->newProcess();
    thread->space = process;
    
    //fork
    thread->Fork (StartForkedProcess, 0);

    return 0;
}

void do_Exit() 
{
    machine->deleteProcess(); // -1

    if (machine->getProcessNumber() == 0) 
    {
        interrupt->Halt();
    }

    //currentThread->space->ToBeDestroyed = true;
    currentThread->Finish();
}