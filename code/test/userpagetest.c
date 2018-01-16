#include "syscall.h"

int main()
{
	//ForkExec("../build/putchar");
	//ForkExec("../build/putchar");
	ForkExec("../build/userpages0");
	ForkExec("../build/userpages1");

	Halt();
	while(1);
}
