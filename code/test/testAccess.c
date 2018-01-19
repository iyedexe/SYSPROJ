#include "syscall.h"

void access (int i) 
{
	if(i == 1)
	{
		char test[1];
		test[1] = 'b';
		Open(test);
		SynchPutString ("Thread, voici l'Id du fichier ouvert : ");
    	SynchPutInt (i);
    	SynchPutString("> \n");
	}
	else
	{
		//Open((char*)"test");
		SynchPutString ("Thread, voici l'Id du fichier ouvert : ");
    	SynchPutInt (i);
    	SynchPutString("> \n");
	}
	UserThreadExit();
}


int main (int argc, char const *argv[]) 
{
    int t1 = UserThreadCreate(access,(void *) 0);
  	int t2 = UserThreadCreate(access,(void *) t1);

  	UserThreadJoin(t2);
    Halt ();
    //return 0;
}

