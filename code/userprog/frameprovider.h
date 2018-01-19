#ifndef FRAMEPROVIDER_H
#define FRAMEPROVIDER_H

//#include "system.h"
//#include "addrspace.h"
#include "bitmap.h"
#include <strings.h>
//#include "filesys.h"
//#include "synch.h"

class FrameProvider
{
    public:

        FrameProvider(int);
        ~FrameProvider();

        int GetEmptyFrame();
        void ReleaseFrame(int frame);
        int NumAvailFrame(void);

    private:
        BitMap* MemBitMap;
        int numberOfFrame; // number of free frames in the bitmap
};

#endif
