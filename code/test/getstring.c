/*test du syscall getchar*/

#include "syscall.h"

int main(){
  char cr[5];
  SynchGetString(cr,5);
  SynchPutString("You just typed : ");
  SynchPutString(cr);
  PutChar('\n');

  Halt();
}
