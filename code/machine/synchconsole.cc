//#ifdef CHANGED

#include "copyright.h"
#include "system.h"
#include "synchconsole.h"
#include "synch.h"

static Semaphore *readAvail;
static Semaphore *writeDone;
static Semaphore *writeMutex;
static Semaphore *readMutex;
static Semaphore *putStringMutex;
static Semaphore *getStringMutex;


static void ReadAvail(int arg) {readAvail->V(); }
static void WriteDone(int arg) {writeDone->V(); }


SynchConsole::SynchConsole(char *readFile , char *writeFile){
  readAvail = new Semaphore("read avail", 0);
  writeDone = new Semaphore("write done", 0);
  writeMutex = new Semaphore("writeMutex", 1);
  readMutex  = new Semaphore("readMutex", 1);
  putStringMutex = new Semaphore("putStringMutex", 1);
  getStringMutex = new Semaphore("getStringMutex", 1);
  console = new Console (readFile, writeFile, ReadAvail, WriteDone, 0);
}

SynchConsole::~SynchConsole(){
  delete console;
  delete writeDone;
  delete readAvail;

}

void SynchConsole::SynchPutChar(const char ch){
  writeMutex->P();
  console->PutChar(ch);
  writeDone->P();
  writeMutex->V();
}

char SynchConsole::SynchGetChar(){
  readMutex->P();
  readAvail->P ();	// wait for character to arrive
  return console->GetChar ();
  readMutex->V();
}

void SynchConsole::SynchPutString(const char s[]){
    putStringMutex->P();

  for(int i=0;i<MAX_STRING_SIZE && s[i]!= '\0';i++){
    this->SynchPutChar((char) s[i]);
  }
    putStringMutex->V();

}

void SynchConsole::SynchGetString(char *s, int n){
  getStringMutex->P();
  if(n>MAX_STRING_SIZE){
    return;
  }
  else{
    char cr;
    int i;
    for(i=0;i<n;i++){
      readAvail->P ();	// wait for character to arrive
      cr = console->GetChar ();
      if (cr == EOF){
        break;
      }
      else if(cr == '\n'){
        s[i] = cr;
        i++;
        break;
      }
      else{
        s[i] = cr;
      }
    }
    s[i] = '\0';
  }
  getStringMutex->V();

}

//#endif //CHANGED
