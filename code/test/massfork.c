#include "syscall.h"

#define NbProcess 3

int main()
{
	int i;
	for(i=0;i<NbProcess;i++){
		ForkExec("../build/massthread");
	}
	//Exit(0);
	Halt();
}
