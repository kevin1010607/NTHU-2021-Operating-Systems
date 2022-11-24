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
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
/*#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		64
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)*/

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
	DEBUG(dbgFile, "Initializing the file system.");
	if (format)
	{
		PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
		Directory *directory = new Directory(NumDirEntries);
		FileHeader *mapHdr = new FileHeader;
		FileHeader *dirHdr = new FileHeader;

		DEBUG(dbgFile, "Formatting the file system.");

		// First, allocate space for FileHeaders for the directory and bitmap
		// (make sure no one else grabs these!)
		freeMap->Mark(FreeMapSector);
		freeMap->Mark(DirectorySector);
		// Second, allocate space for the data blocks containing the contents
		// of the directory and bitmap files.  There better be enough space!

		ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
		ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));
		// Flush the bitmap and directory FileHeaders back to disk
		// We need to do this before we can "Open" the file, since open
		// reads the file header off of disk (and currently the disk has garbage
		// on it!).

		DEBUG(dbgFile, "Writing headers back to disk.");
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

		DEBUG(dbgFile, "Writing bitmap and directory back to disk.");

		freeMap->WriteBack(freeMapFile); // flush changes to disk

		directory->WriteBack(directoryFile);

		if (debug->IsEnabled('f'))
		{
			freeMap->Print();
			directory->Print();
		}
		delete freeMap;
		delete directory;
		delete mapHdr;
		delete dirHdr;
	}
	else
	{
		// if we are not formatting the disk, just open the files representing
		// the bitmap and directory; these are left open while Nachos is running
		freeMapFile = new OpenFile(FreeMapSector);
		directoryFile = new OpenFile(DirectorySector);
	}
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileSystem::~FileSystem
//----------------------------------------------------------------------
FileSystem::~FileSystem()
{
	delete freeMapFile;
	delete directoryFile;
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
void FileSystem::SplitPath(char *FullPath, char *Path, char *filename)
{
	int i, j;
	int split;
	// /t0/bb/f1
	for (i = 0; FullPath[i] != '\0'; i++)
	{
		if (FullPath[i] == '/')
		{
			split = i; // last '/'
		}
	}
	Path[0] = '/';
	for (i = 1; i < split; i++)
	{
		Path[i] = FullPath[i];
	}
	Path[i] = '\0';
	for (i = split, j = 0; FullPath[i] != '\0'; i++, j++)
	{
		filename[j] = FullPath[i];
	}
	filename[j] = '\0';
}

void FileSystem::CreateDirectory(char *name)
{
	int DirecSector, sector;
	Directory *RootDirectory = new Directory(NumDirEntries);
	Directory *directory = new Directory(NumDirEntries);
	OpenFile *file;
	PersistentBitmap *freeMap;
	FileHeader *hdr;
	bool success;

	int size = DirectoryFileSize;
	char Path[256];
	char filename[10];
	DEBUG(dbgFile, "Creating Directory " << name << " size " << DirectoryFileSize);

	RootDirectory->FetchFrom(directoryFile);

	SplitPath(name, Path, filename);
	// /t0/bb/f1 -> path = /t0/bb , filename = /f1

	DirecSector = RootDirectory->FindPath(Path);
	if (DirecSector == -1)
		return;

	file = new OpenFile(DirecSector);
	directory->FetchFrom(file);

	directory->Find(filename);

	freeMap = new PersistentBitmap(freeMapFile, NumSectors);
	sector = freeMap->FindAndSet(); // find a sector to hold the file header
	directory->Add(filename, sector, true);

	hdr = new FileHeader;
	hdr->Allocate(freeMap, size);
	hdr->WriteBack(sector);
	directory->WriteBack(file);
	freeMap->WriteBack(freeMapFile);

	delete file;
	delete directory;
	file = new OpenFile(sector);
	directory = new Directory(NumDirEntries);
	directory->WriteBack(file);
	delete hdr;
	delete freeMap;
	delete file;
	delete RootDirectory;
	delete directory;
}

bool FileSystem::Create(char *name, int initialSize)
{
	int DirecSector, sector;
	Directory *RootDirectory = new Directory(NumDirEntries);
	Directory *directory = new Directory(NumDirEntries);
	OpenFile *file;
	PersistentBitmap *freeMap;
	FileHeader *hdr;
	bool success;

	char Path[256];
	char filename[10];
	DEBUG(dbgFile, "Creating file " << name << " size " << initialSize);

	RootDirectory->FetchFrom(directoryFile);

	SplitPath(name, Path, filename);
	// /t0/bb/f1 -> path = /t0/bb , filename = /f1

	DirecSector = RootDirectory->FindPath(Path);
	if (DirecSector == -1)
		return FALSE;
	file = new OpenFile(DirecSector);
	directory->FetchFrom(file);

	if (directory->Find(filename) != -1)
		success = FALSE; // file is already in directory
	else
	{
		freeMap = new PersistentBitmap(freeMapFile, NumSectors);
		sector = freeMap->FindAndSet(); // find a sector to hold the file header
		if (sector == -1)
			success = FALSE; // no free block for file header
		else if (!directory->Add(filename, sector, false))
			success = FALSE; // no space in directory
		else
		{
			hdr = new FileHeader;
			if (!hdr->Allocate(freeMap, initialSize))
				success = FALSE; // no space on disk for data
			else
			{
				success = TRUE;
				// everthing worked, flush all changes back to disk
				hdr->WriteBack(sector);
				directory->WriteBack(file);
				freeMap->WriteBack(freeMapFile);
			}
			delete hdr;
		}
		delete freeMap;
	}
	delete file;
	delete RootDirectory;
	delete directory;
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
FileSystem::Open(char *name)
{
	Directory *directory = new Directory(NumDirEntries);
	OpenFile *openFile = NULL;
	int sector;

	DEBUG(dbgFile, "Opening file" << name);
	directory->FetchFrom(directoryFile);
	sector = directory->FindPath(name);
	if (sector >= 0)
		openFile = new OpenFile(sector); // name was found in directory
	delete directory;
	return openFile; // return NULL if not found
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

char *string_concat(char *str1, char *str2) {  
   int length=strlen(str1)+strlen(str2)+1;  

   char *result = (char*)malloc(sizeof(char) * length);  
       
   strcpy(result, str1);  
 
   strcat(result, str2);  
     
   return result;  
}  

bool FileSystem::Remove(char *name)
{
	Directory *directory;
	PersistentBitmap *freeMap;
	FileHeader *fileHdr;
	int sector;
	char *temp="/";

	directory = new Directory(NumDirEntries);
	directory->FetchFrom(directoryFile);
	/* get the first token */
	OpenFile *file_tem = directoryFile;
	OpenFile *prev = file_tem;
	char *token = strtok(name, "/"); // /t0 or name
	char *token_prev;
	token_prev = token;
	bool file = false;
	
	while (token != NULL)
	{
		token = string_concat(temp,token);
		if ((sector = directory->Find(token)) == -1 || !directory->IsDirectory(token))
		{
			file = true;
			break;
		}
		prev = file_tem;
		file_tem = new OpenFile(sector);
		directory->FetchFrom(file_tem);
		token_prev = token;
		token = strtok(NULL, "/"); // /t1 or name
		
	}
	if (token == NULL)
		name = token_prev;
	else
		name = token;

	if (file)
		directory->FetchFrom(file_tem);
	else
		directory->FetchFrom(prev);
	
	sector = directory->Find(name);

	if (sector == -1)
	{
		delete directory;
		return FALSE; // file not found
	}
	fileHdr = new FileHeader;
	fileHdr->FetchFrom(sector);

	freeMap = new PersistentBitmap(freeMapFile, NumSectors);

	fileHdr->Deallocate(freeMap); // remove data blocks
	freeMap->Clear(sector);		  // remove header block
	directory->Remove(name);

	freeMap->WriteBack(freeMapFile); // flush to disk
	if (file)
		directory->WriteBack(file_tem); // flush to disk
	else
		directory->WriteBack(prev);
	delete fileHdr;
	delete directory;
	delete freeMap;
	return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List(char *name)
{
	Directory *directory = new Directory(NumDirEntries);
	OpenFile *file = NULL;

	directory->FetchFrom(directoryFile);
	int sector = directory->FindPath(name);
	if (sector >= 0)
		file = new OpenFile(sector);

	directory->FetchFrom(file);
	directory->List();

	delete file;
	delete directory;
}

void FileSystem::RecursiveList(char *name)
{
	int sector;
	char *temp="/";
	Directory *directory = new Directory(NumDirEntries);
	directory->FetchFrom(directoryFile);

	OpenFile *file_tem = directoryFile;
	char *token = strtok(name, "/");
	
	while (token != NULL)
	{
		token = string_concat(temp,token);
		if ((sector = directory->Find(token)) == -1)
			break;
		file_tem = new OpenFile(sector);
		directory->FetchFrom(file_tem);
		token = strtok(NULL, "/");
		
	}

	directory->RecursiveList();

	if (token != NULL)
	{
		delete directory;
		delete token;
		delete file_tem;
	}
}

bool FileSystem::RecursiveRemove(char *name)
{
	Directory *directory;
	PersistentBitmap *freeMap;
	FileHeader *fileHdr;
	int sector;
	char *temp="/";

	directory = new Directory(NumDirEntries);
	directory->FetchFrom(directoryFile);

	OpenFile *file_tem = directoryFile;
	OpenFile *file_dir;

	char pwd[20], buffer[20];
	strcpy(pwd, name);

	char *token = strtok(name, "/");
	char *token_prev;
	token_prev = token;

	while (token != NULL)
	{
		 token = string_concat(temp,token);
		if ((sector = directory->Find(token)) == -1 || !directory->IsDirectory(token)){
			break;
		}
		file_tem = new OpenFile(sector);
		directory->FetchFrom(file_tem);
		token_prev = token;
		token = strtok(NULL, "/");
	}
	if (token == NULL)
		name = token_prev;
	else
		name = token;
	DirectoryEntry *table = directory->getTable();

	if (directory->IsDirectory(name))
	{
		for (int i = 0; i < directory->getTableSize(); i++){
			if (table[i].inUse)
			{
				if (table[i].isDirectory)
				{
					int n = sprintf(buffer, "%s/%s", pwd, table[i].name);
					RecursiveRemove(buffer);
				}
				else
				{
					sector = directory->Find(table[i].name);
					fileHdr = new FileHeader;
					fileHdr->FetchFrom(sector);
					freeMap = new PersistentBitmap(freeMapFile, NumSectors);
					fileHdr->Deallocate(freeMap);
					freeMap->Clear(sector);
					directory->Remove(table[i].name);
					freeMap->WriteBack(freeMapFile);
					directory->WriteBack(file_tem);
				}
			}
		}
		Remove(pwd);
	}
	else
	{
		printf("IS FILE %s\n", name);
		sector = directory->Find(name);
		fileHdr = new FileHeader;
		fileHdr->FetchFrom(sector);

		freeMap = new PersistentBitmap(freeMapFile, NumSectors);
		fileHdr->Deallocate(freeMap);
		freeMap->Clear(sector);
		directory->Remove(name);

		freeMap->WriteBack(freeMapFile);
		directory->WriteBack(file_tem);
	}

	//delete fileHdr;
	//delete directory;
	//delete freeMap;
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

void FileSystem::Print()
{
	FileHeader *bitHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;
	PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile, NumSectors);
	Directory *directory = new Directory(NumDirEntries);

	printf("Bit map file header:\n");
	bitHdr->FetchFrom(FreeMapSector);
	bitHdr->Print();

	printf("Directory file header:\n");
	dirHdr->FetchFrom(DirectorySector);
	dirHdr->Print();

	freeMap->Print();

	directory->FetchFrom(directoryFile);
	directory->Print();

	delete bitHdr;
	delete dirHdr;
	delete freeMap;
	delete directory;
}

#endif // FILESYS_STUB
