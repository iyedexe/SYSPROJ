#include "frameprovider.h"

FrameProvider::FrameProvider(int nFrame)
{
  this->bitmap = new BitMap(nFrame);
  this->numberOfFrame = bitmap->NumClear();

  if(this->numberOfFrame != nFrame)
    fprintf(stderr, "%s", "Error while initialising the number of free frame(s).\n");
}

FrameProvider::~FrameProvider()
{
  delete bitmap;
}

int 
FrameProvider::NumAvailFrame()
{
  return numberOfFrame;
}

int 
FrameProvider::GetEmptyFrame()
{
  if(NumAvailFrame() == 0){
    fprintf(stderr, "%s", "No space remaining.\n");
  }

  int frame = this->bitmap->Find(); //return -1 if full

  this->bitmap->Mark(frame);
  bzero(&(machine->mainMemory[PageSize * frame]), PageSize);

  return frame;

}
        
void 
FrameProvider::ReleaseFrame(int framePosition)
{
  bitmap->Clear(framePosition);
}

