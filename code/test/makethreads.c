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
        UserThreadCreate(print,(void *) 13);
        UserThreadCreate(print,(void *) 22);
        UserThreadCreate(print,(void *) 17);
        UserThreadCreate(print,(void *) 19);

//ISSUE : More than three threads there is a problem in the display

        Halt();
        return 0 ;
}
