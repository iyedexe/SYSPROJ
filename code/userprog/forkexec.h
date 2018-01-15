#include "system.h"
#include "thread.h"
#include "sysdep.h"

#ifndef FORKEXEC_H
#define FORKEXEC_H

struct SpaceContainer{
	AddrSpace* space;
};

extern int do_ForkExec (char *filename);
extern void do_Exit ();

#endif
