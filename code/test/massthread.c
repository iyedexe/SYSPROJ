#include "syscall.h"

#define NbThread 3

void thread(int *i){
	if(*i!=-1){
//		UserThreadJoin(i[0]);
		SynchPutString("Thread ");
		SynchPutInt(i[0]);
		PutChar('\n');
	}
	else{
		SynchPutString("Thread initial\n");
	}
	//Calcul sale pour "ralentir" les threads
	int a=1001;
	int j;
	for(j=0;j<2000;j++){
		if(a%2){
			a=a*2;
		}
		else{
			a=a/2;
		}
	}
	UserThreadExit();
}

int main()
{
	int i,tmp,param[NbThread+1];
	param[0]=-1;
	tmp=UserThreadCreate(thread,(void *)(param));
	param[1]=tmp;
	for(i=1;i<NbThread;i++){
		tmp=UserThreadCreate(thread,(void *)(param+i));
		param[i+1]=tmp;
}
	Halt();
}
