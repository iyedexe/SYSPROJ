 #include "syscall.h"

 void thread(int i){
   if (i!= 0){
     UserThreadJoin(i);
     SynchPutString("waiting for Thread : <");
     SynchPutInt(i);
     SynchPutString("> \n");
   }
   else{ // first thread execution
     SynchPutString("Initial Thread ::");
     int a = 1001;
     for(int j=0;j<1000;j++){
       if (a%2){
         a = a*2;
       }
       else{
         a = a/2;
       }
     }
   }
   UserThreadExit();

 }

int main(){
  //int param = 1;
  int t1 = UserThreadCreate(thread,(void *) 0);
  int t2 = UserThreadCreate(thread,(void *) t1);
  /*SynchPutString("t1 :<");
  SynchPutInt(t1);
  SynchPutString(">\n");

  SynchPutString("t2 :<");
  SynchPutInt(t2);
  SynchPutString(">\n");
*/
  UserThreadJoin(t2);
  SynchPutString("Main program terminated\n");
  Halt();
}
