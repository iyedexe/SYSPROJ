
#ifdef CHANGED

#ifndef USERTHREAD_H
#define USERTHREAD_H


typedef struct{
  int f;
  int arg;
}forkArgs;

extern int do_UserThreadCreate(int f, int arg);
extern void do_UserThreadExit();
extern int do_UserThreadJoin(int tid);

#endif
#endif //CHANGED
