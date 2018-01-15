/*test du syscall getchar*/

#include "syscall.h"

int main(){
  char cr = SynchGetChar();
  SynchPutString("You just typed : ");
  PutChar(cr);
  PutChar('\n');

  Halt();
}
