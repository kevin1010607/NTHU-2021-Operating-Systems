// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include "filesys.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
	table = new DirectoryEntry[size];

	// MP4 mod tag
	memset(table, 0, sizeof(DirectoryEntry) * size); // dummy operation to keep valgrind happy

	tableSize = size;
	for (int i = 0; i < tableSize; i++)
		table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
	delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
	(void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
	(void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{

	for (int i = 0; i < tableSize; i++){
		//cout << table[i].name << " " << name << endl;
		if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
			return i;
	}
	return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::Find(char *name)
{
	int i = FindIndex(name);

	if (i != -1)
		return table[i].sector;
	return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector, bool Directory)
{
	if (FindIndex(name) != -1)
		return FALSE; // name is in Directory

	for (int i = 0; i < tableSize; i++)
		if (!table[i].inUse)
		{
			table[i].inUse = TRUE;
			strncpy(table[i].name, name, FileNameMaxLen);
			table[i].sector = newSector;
			table[i].isDirectory = Directory;
			return TRUE;
		}
	return FALSE; // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name)
{
	int i = FindIndex(name);

	if (i == -1)
		return FALSE; // name not in directory
	table[i].inUse = FALSE;
	return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------

void Directory::List()
{
	int counter = 0;
	char type;
	for (int i = 0; i < tableSize; i++)
		if (table[i].inUse)
		{
			if (table[i].isDirectory)
				type = 'D';
			else
				type = 'F';
			printf("[%d] %s %c\n", counter, table[i].name, type);
			counter++;
		}
}

void Directory::RecursiveList()
{
	Directory *subDirectory = new Directory(NumDirEntries);
	OpenFile *file_tem;

	int counter = 0;
	char type;
	for (int i = 0; i < tableSize; i++)
		if (table[i].inUse)
		{
			if (table[i].isDirectory) // the element is a directory or not
				type = 'D';
			else
				type = 'F';
			printf("[%d] %s %c\n", counter, table[i].name, type); // print the information
			if (table[i].isDirectory)
			{
				file_tem = new OpenFile(table[i].sector); // go to next directory
				subDirectory->FetchFrom(file_tem);
				subDirectory->RecursiveList();
			}
			counter++;
		}
	//delete subDirectory;
	//delete file_tem;
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print()
{
	FileHeader *hdr = new FileHeader;

	printf("Directory contents:\n");
	for (int i = 0; i < tableSize; i++)
		if (table[i].inUse)
		{
			printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
			hdr->FetchFrom(table[i].sector);
			hdr->Print();
		}
	printf("\n");
	delete hdr;
}

int Directory::FindPath(char *name)
{

	char rootPath[256];
	char restPath[256];

	if (name[1] == '\0')
	{
		return 1; // root, return directory sector
	}
	else
	{
		bool LastLevel = true;
		int split = 0;
		// -> /t0/bb
		for (int i = 1; name[i] != '\0'; i++)
		{
			if (name[i] == '/')
			{
				strncpy(rootPath, name, i); // copy ith words
				rootPath[i] = '\0';			// rootPath -> /t0
				LastLevel = false;
				split = i;
				break;
			}
		}
		if (LastLevel)
		{
			strcpy(rootPath, name);
			strcat(rootPath, "\0");
			return Find(rootPath);
		}
		else
		{
			int sector = Find(rootPath);
			if (sector == -1)
				return -1;
			Directory *directory = new Directory(DirectoryFileSize); // 9+1
			OpenFile *directoryFile = new OpenFile(sector);
			directory->FetchFrom(directoryFile);
			/*for(int i = 0; name[split+i] != '\0'; i++)
				restPath[i] = name[split+i];
			*/
			for (int i = 1; i < strlen(name); i++)
			{
				if (name[i] == '/')
				{
					for (int j = 0; j < strlen(name) - i + 1; j++)
						restPath[j] = name[j + i];
					break;
				}
			}
			sector = directory->FindPath(restPath);
			delete directory;
			delete directoryFile;
			return sector;
		}
	}
}

bool Directory::IsDirectory(char *name)
{
	int i = FindIndex(name);
	if (i != -1)
		return table[i].isDirectory;
	return -1;
}
