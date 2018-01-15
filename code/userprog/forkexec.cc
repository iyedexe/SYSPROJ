#include "forkexec.h"
#include "addrspace.h"


static void StartForkedProcess(int arg) {
    SpaceContainer* sarg = (SpaceContainer*) arg; // on restaure notre sérialisation
  	currentThread->space = sarg->space; // on affecte le nouvel espace mémoir à notre nouveau processus


    currentThread->space->RestoreState();
    currentThread->space->InitRegisters();
    //currentThread->space->InitMainThread();
    machine->Run();
}

int do_ForkExec (char *filename)
{
    OpenFile *executable = fileSystem->Open (filename);
    AddrSpace *space = new AddrSpace(executable); // création du nouvel espace mémoir du processus que l'on va mettre en place

    Thread* newThread = new Thread("newProcess"); // un processus est juste un thread avec un nouvel espace mémoir
    if (executable == NULL)
    {
        fprintf(stderr, "%s", "Error when opening the file\n");
        return -1;
    }

    //Process
    if(space == NULL)
    {
        fprintf(stderr, "%s", "Error when allocating the memory for the process\n");
        return -1;
    }

    if(space->getSpaceAllocation() == -1)
    {
        fprintf(stderr, "%s", "Not enough space for the process\n");
        return -1;
    }

    SpaceContainer* sarg = new SpaceContainer; // comme pour les threads, on sérialise l'espace mémoir qu'on souhaite affecter à notre processus
    sarg->space = space;
    newThread->Fork(StartForkedProcess,(int)sarg); // on fork le processus père

    machine->newProcess();
    delete space;
    delete executable;
    
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
