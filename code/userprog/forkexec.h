#include "system.h"
#include "thread.h"
#include "sysdep.h"

#ifndef FORKEXEC_H
#define FORKEXEC_H

struct SpaceContainer{
	AddrSpace* space;
};

//extern void StartForkedProcess (int arg);
extern int do_ForkExec (char *filename);
extern void do_Exit ();

#endif
