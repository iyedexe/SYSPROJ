// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "system.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)
#define DIR_NUMBER_PATH 10
#define DIR_NAME_MAX 50
#define FILE_NAME_MAX 50

#define MAX 10

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
	FileHeader *mapHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

    // First, allocate space for FileHeaders for the directory and bitmap
    // (make sure no one else grabs these!)
	freeMap->Mark(FreeMapSector);	    
	freeMap->Mark(DirectorySector);

    // Second, allocate space for the data blocks containing the contents
    // of the directory and bitmap files.  There better be enough space!

	ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize, 1, 0));
	ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize, 2, 1));

    // Flush the bitmap and directory FileHeaders back to disk
    // We need to do this before we can "Open" the file, since open
    // reads the file header off of disk (and currently the disk has garbage
    // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
	mapHdr->WriteBack(FreeMapSector);    
	dirHdr->WriteBack(DirectorySector);

    // OK to open the bitmap and directory files now
    // The file system operations assume these two files are left open
    // while Nachos is running.

    freeMapFile = new OpenFile(FreeMapSector);
    directoryFile = new OpenFile(DirectorySector);
     
    // Once we have the files "open", we can write the initial version
    // of each file back to disk.  The directory at this point is completely
    // empty; but the bitmap has been changed to reflect the fact that
    // sectors on the disk have been allocated for the file headers and
    // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
	freeMap->WriteBack(freeMapFile);	 // flush changes to disk
	directory->WriteBack(directoryFile);

	if (DebugIsEnabled('f')) {
	    freeMap->Print();
	    directory->Print();

        delete freeMap; 
	delete directory; 
	delete mapHdr; 
	delete dirHdr;
	}
    } else {
    // if we are not formatting the disk, just open the files representing
    // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }

    strcat(this->dir, "/");
    for(int i = 0; i < OPEN_FILE_MAX; i++)
        OpenedFilesTracker[i] = 0;
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(const char *nm, int initialSize)
{
    Directory* directory;
    Directory *parentDirectory;
    BitMap *freeMap;
    FileHeader *hdr;
    int path_number = 0;
    bool success;
    char parsed_path[DIR_NUMBER_PATH][FILE_NAME_MAX];

    char name[FILE_NAME_MAX];
    strcpy(name, nm);

    if(strlen(name) > FILE_NAME_MAX)
    {
        fprintf(stderr, "%s\n", "The name of the file is too long");
        return false;
    }

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    int old_sector = directory->getTableZero().sector;

    if(!this->goToParent(name, parsed_path, path_number))
    {
        fprintf(stderr, "%s\n", "Failed to reach the parent folder");
        delete directory;
        return false;
    }

    parentDirectory = new Directory(NumDirEntries);
    parentDirectory->FetchFrom(this->directoryFile);

    if(parentDirectory->getDirSpaceRemaining() == 0) 
    {
        fprintf(stderr, "%s\n", "The parent folder is full");
        delete parentDirectory;
        delete directory;
        return false;
    }

    if(parentDirectory->Find(name) != -1)
    {
        fprintf(stderr, "%s\n", "The folder already exists");
        delete parentDirectory;
        delete directory;
        return false;
    }
	
    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);
    int new_sector = freeMap->Find();	// find a sector to hold the file header

    if(new_sector == -1)
    {
        fprintf(stderr, "%s\n", "No bit are free");
        delete freeMap;
        delete parentDirectory;
        delete directory;
        return false;
    }

    if(!parentDirectory->Add(parsed_path[path_number - 1], new_sector))
        success = false;
    else
    {   
        hdr = new FileHeader;
        if(!hdr->Allocate(freeMap, initialSize, 1, new_sector))
            success = false;

        parentDirectory->minSpaceRemaining();

        hdr->WriteBack(new_sector);
        parentDirectory->WriteBack(directoryFile);
        freeMap->WriteBack(freeMapFile);

        success = true;
    }

    delete freeMap;
    delete hdr;
    delete directory;
    delete parentDirectory;
        
    //go to the old position
    OpenFile* fileDirectory = new OpenFile(old_sector);
    directoryFile = fileDirectory;

    delete fileDirectory;

    printf("succes = %d\n", success);
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(const char *name)
{ 
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Opening file %s\n", name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name); 
    if (sector >= 0) // name was found in directory
    {
        if(!findValue(sector))
        {
            openFile = new OpenFile(sector);
            currentThread->addFile(sector);
            currentThread->count_file +=1;
        }
    } 		
		 
    delete directory;
    return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(const char *name)
{ 
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    
    if (sector == -1) {
       delete directory;
       return FALSE;			 // file not found 
    }

    OpenFile* new_file = new OpenFile(directory->getTablePos(0).sector);
    if(new_file->Length() == DirectoryFileSize)
    {
        if(directory->getDirSpaceRemaining() != NumDirEntries - 2)
        {
            fprintf(stderr, "%s\n", "The folder is not empty");
            return FALSE;
        }
    }


    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(directoryFile);        // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List(char* space)
{
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    OpenFile* former_file;
    OpenFile* new_file;

    int old_sector = directory->getTableZero().sector;

    char sub[500];
    strcat(space, "    ");
    strcpy(sub, space);

    printf("%s%s\n", space, directory->getTablePos(0).name);
    printf("%s%s\n", space, directory->getTablePos(1).name);
    for (int i = 2; i < directory->getTableSize(); i++)
    {
        if (directory->getTablePos(i).inUse)
        {
            new_file = new OpenFile(directory->getTablePos(i).sector);

            if(new_file->Length() != DirectoryFileSize)
                printf("%s%s : file\n", space, directory->getTablePos(i).name);
            else
                printf("%s%s : folder\n", space, directory->getTablePos(i).name);
            
            if(new_file->Length() == DirectoryFileSize)
            {
                goIntoDir(directory->getTablePos(i).name);
                List(space); 
            }
        }
        former_file = new OpenFile(old_sector);
        this->directoryFile = former_file;
        strcpy(space, sub);
    }

   former_file = new OpenFile(old_sector);

   this->directoryFile = former_file;
   delete directory;
}


//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}

//step 5
bool 
FileSystem::goIntoDir(char *name) //cd 
{

    //get the full directory
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(this->directoryFile);

    //test if the folder exists
    int sector = directory->Find(name);

    if(sector == -1)
    {
        fprintf(stderr, "%s\n", "Directory not found.");
        delete directory;
        return false;
    }

    //test if it is folder
    OpenFile* newDirectory = new OpenFile(sector);

    if(newDirectory->Length() != DirectoryFileSize)
    {
        fprintf(stderr, "%s\n", "This is not a folder");
        delete directory;
        delete newDirectory;
        return false;
    }

    this->directoryFile = newDirectory;

    delete directory;
    //delete newDirectory;

    strcat(this->dir, name);
    strcat(this->dir, "/");
    //printf("\nposition = %s\n", this->dir);
    
    return true;
}

bool 
FileSystem::parsing(char* path, char parsed[][DIR_NAME_MAX], int &path_number)
{
    char* result = path;
    int i = 0;

    if(path[0] == '/')
        strsep(&result, "/");

    while(result != NULL)
    {
        if(i < DIR_NUMBER_PATH - 1)
        {
           strcpy(parsed[i], strsep(&result, "/"));
           i += 1; 
        }
        else
        {
            fprintf(stderr, "%s\n", "The path contain too many access");
            return false;
        }
    }

    path_number = i;
    return true;
}

bool 
FileSystem::goToParent(char* name, char parsed_path[][DIR_NAME_MAX], int &path_number)
{
    OpenFile* new_position;

    int verif = false;
    for(int i = 0; i < DIR_NAME_MAX; i++)
    {
        if(!name[i] == '\0')
        {
            if(name[i] == '/')
                verif = true;
        }
        else
            break;
    }

    //if it is not a path "mkdir blabla" -> it is done it the current folder
    if(verif == false)
    {
        printf("Current folder\n");
        strcpy(parsed_path[0], name);
        path_number = 1;
        return true;
    }
    //if it is a path -> parse
    else
    {
        printf("Path folder\n");

        char sub[DIR_NAME_MAX];
        strcpy(sub, name);

        char* tempo = new char[DIR_NAME_MAX];
        strcpy(tempo, name);
        //from the root
        if (name[0] == '/') 
        {
            new_position = new OpenFile(1);
            this->directoryFile = new_position;
            strsep(&tempo, "/");
            strcpy(name, tempo);
        }

        if(!parsing(sub, parsed_path, path_number))
        {
            fprintf(stderr, "%s\n", "Error during the parsing");
            return false;
        }

        printf("path_number = %d\n", path_number);

        if(path_number == 1)
            return true;

        for(int i = 0; i < path_number - 1; i++)
        {
            printf("%s\n", parsed_path[i]);
            if(!goIntoDir(parsed_path[i]))
            {
                fprintf(stderr, "%s\n", "Error in the path");
                return false;
            }
        }
    }

    return true;
}

bool
FileSystem::mkdir(char* name)
{
    BitMap *freeMap;
    FileHeader *hdr;
    Directory *directory;
    Directory *parentDirectory;
    Directory *newDirectory;
    OpenFile *fileDirectory;
    int success;
    int path_number = 0;
    char parsed_path[DIR_NUMBER_PATH][DIR_NAME_MAX];

    if(strlen(name) > DIR_NAME_MAX)
    {
        fprintf(stderr, "%s\n", "The name of the folder is too long");
        return false;
    }

    //get the full directory
    directory = new Directory(NumDirEntries);

    //save of the current position
    int old_sector = directory->getTableZero().sector;

    //move to the wanted folder
    if(!this->goToParent(name, parsed_path, path_number))
    {
        fprintf(stderr, "%s\n", "Failed to reach the parent folder");
        delete directory;
        return false;
    }

    // Parent directory
    parentDirectory = new Directory(NumDirEntries);
    parentDirectory->FetchFrom(this->directoryFile);

    if(parentDirectory->getDirSpaceRemaining() == 0) 
    {
        fprintf(stderr, "%s\n", "The parent folder is full");
        delete parentDirectory;
        delete directory;
        return false;
    }

    if(parentDirectory->Find(name) != -1)
    {
        fprintf(stderr, "%s\n", "The folder already exists");
        delete parentDirectory;
        delete directory;
        return false;
    }

    //Allocation for the new folder
    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    int new_sector = freeMap->Find();
    printf("new_sector == %d\n", new_sector);

    if(new_sector == -1)
    {
        fprintf(stderr, "%s\n", "No bit are free");
        delete freeMap;
        delete parentDirectory;
        delete directory;
        return false;
    }

    //make directory
    int parent_sector = parentDirectory->getTableZero().sector;

    if(!parentDirectory->Add(parsed_path[path_number - 1], new_sector))
        success = false;
    else
    {   
        hdr = new FileHeader;
        if(!hdr->Allocate(freeMap, DirectoryFileSize, 2, new_sector))
            success = false;

        parentDirectory->minSpaceRemaining();
        newDirectory = new Directory(NumDirEntries, new_sector, parent_sector);
        fileDirectory = new OpenFile(new_sector); //needed for WriteBack(OpenFile*)
        
        fileDirectory->getFileHeader()->setHdrType(2);

        hdr->WriteBack(new_sector);
        newDirectory->WriteBack(fileDirectory);
        parentDirectory->WriteBack(directoryFile);
        freeMap->WriteBack(freeMapFile);

        success = true;
    }

    delete freeMap;
    delete hdr;
    delete directory;
    delete parentDirectory;
    delete newDirectory;
        
    //go to the old position
    fileDirectory = new OpenFile(old_sector);
    directoryFile = fileDirectory;

    delete fileDirectory;

    return success;
}

bool 
FileSystem::addFile(int sector)
{
    for(int i = 0; i < OPEN_FILE_MAX; i++)
    {
        if(OpenedFilesTracker[i] == 0)
        {
            //printf("File in sector %d added\n", sector);
            OpenedFilesTracker[i] = sector;
            return TRUE;
        }
    }

    fprintf(stderr, "%s\n", "You reached the maximum number of opened files in the same time");
    return FALSE;
}

bool 
FileSystem::rmFile(int sector)
{
    for(int i = 0; i < OPEN_FILE_MAX; i++)
    {
        if(OpenedFilesTracker[i] == sector)
        {
            OpenedFilesTracker[i] = 0;
            return TRUE;
        }
    }

    fprintf(stderr, "%s\n", "The sector is not used by a file");
    return FALSE;
}

bool 
FileSystem::findValue(int sector)
{
    for(int i = 0; i < OPEN_FILE_MAX; i++)
    {
        if(OpenedFilesTracker[i] == sector)
        {
            return TRUE;
        }
    }
    return FALSE;
}

