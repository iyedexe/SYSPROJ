#include "frameprovider.h"
#include "synch.h"
#include "system.h"

static Semaphore* semMemBitMap = new Semaphore("semFrame",1);

FrameProvider::FrameProvider(int nFrame)
{
  this->MemBitMap = new BitMap(nFrame);
  this->numberOfFrame = MemBitMap->NumClear();

  if(this->numberOfFrame != nFrame)
    fprintf(stderr, "%s", "Error while initialising the number of free frame(s).\n");
}

FrameProvider::~FrameProvider()
{
  delete MemBitMap;
}

int
FrameProvider::NumAvailFrame()
{
  return MemBitMap->NumClear();
}

int
FrameProvider::GetEmptyFrame()
{
  semMemBitMap->P();
  if(MemBitMap->NumClear() <= 0){
    semMemBitMap->V();
    fprintf(stderr, "%s", "No space remaining.\n");
    return -1;
  }

  int frame = MemBitMap->Find(); //return -1 if full

  bzero(&(machine->mainMemory[PageSize * frame]), PageSize);
  semMemBitMap->V();
  return frame;

}

void
FrameProvider::ReleaseFrame(int framePosition)
{
  MemBitMap->Clear(framePosition);
}
