/*test du syscall getchar*/

#include "syscall.h"

int main(){
  int i =0;
  SynchGetInt(&i);
  SynchPutString("You just typed : ");
  SynchPutInt(i);
  PutChar('\n');

  Halt();
}
