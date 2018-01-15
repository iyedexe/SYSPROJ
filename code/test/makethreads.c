#include "syscall.h"

void print(int i) {

    if (i %2)
{
        SynchPutString("And I'm Steven\n");
//          SynchPutChar('A');
}
else
{
        SynchPutString("I'm Evenovski\n");
          //SynchPutChar('B');
}
        UserThreadExit();
}
int main() {
        SynchPutString("let's begin\n");
        int t1 = UserThreadCreate(print,(void *) 0);
        UserThreadCreate(print,(void *) 1);
        UserThreadCreate(print,(void *) 2);
        UserThreadJoin(t1);
        SynchPutString("<<Main waited>>\n");
        UserThreadCreate(print,(void *) 4);

/*        UserThreadCreate(print,(void *) 19);
        UserThreadCreate(print,(void *) 17);
        UserThreadCreate(print,(void *) 19);
        UserThreadCreate(print,(void *) 17);
        UserThreadCreate(print,(void *) 19);*/
//ISSUE : More than three threads there is a problem in the display

        Halt();
        return 0 ;
}
