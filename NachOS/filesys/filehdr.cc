// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"
// Andrew
#define Limit2 NumDirect *SectorSize
#define Limit3 NumDirect *NumDirect *SectorSize
#define Limit4 NumDirect *NumDirect *NumDirect *SectorSize
//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader()
{
	numBytes = -1;
	numSectors = -1;
	memset(dataSectors, -1, sizeof(dataSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	// nothing to do now
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
	numBytes = fileSize;
	numSectors = divRoundUp(fileSize, SectorSize); // total # of sectors
	if (freeMap->NumClear() < numSectors)
	{
		return FALSE; // not enough space
	}
	else
	{
		// 1st level header
		if (fileSize > Limit4)
		{
			DEBUG('f', "4th-level");

			int i = 0;
			while (fileSize > 0)
			{
				dataSectors[i] = freeMap->FindAndSet();
				FileHeader *subhdr = new FileHeader;
				if (fileSize > Limit4)
					subhdr->Allocate(freeMap, Limit4);
				else
					subhdr->Allocate(freeMap, fileSize);
				fileSize -= Limit4;
				subhdr->WriteBack(dataSectors[i]);
				i++;
			}
		}
		// 2nd level header
		else if (fileSize > Limit3)
		{
			DEBUG('f', "3rd-level");
			int i = 0;
			while (fileSize > 0)
			{
				dataSectors[i] = freeMap->FindAndSet();
				FileHeader *subhdr = new FileHeader;
				if (fileSize > Limit3)
					subhdr->Allocate(freeMap, Limit3);
				else
					subhdr->Allocate(freeMap, fileSize);
				fileSize -= Limit3;
				subhdr->WriteBack(dataSectors[i]);
				i++;
			}
		}
		// 3rd level header
		else if (fileSize > Limit2)
		{
			DEBUG('f', "2nd-level");
			int i = 0;
			while (fileSize > 0)
			{
				dataSectors[i] = freeMap->FindAndSet();
				FileHeader *subhdr = new FileHeader;
				if (fileSize > Limit2)
					subhdr->Allocate(freeMap, Limit2);
				else
					subhdr->Allocate(freeMap, fileSize);
				fileSize -= Limit2;
				subhdr->WriteBack(dataSectors[i]);
				i++;
			}
		}
		// 4th level header
		else
		{
			DEBUG('f', "1st-level");
			numBytes = fileSize;
			numSectors = divRoundUp(fileSize, SectorSize);
			for (int i = 0; i < numSectors; i++)
			{
				dataSectors[i] = freeMap->FindAndSet();
				// since we checked that there was enough free space,
				// we expect this to succeed
				ASSERT(dataSectors[i] >= 0);
			}
		}
		return TRUE;
	}
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
	// 1st level
	if (numBytes > Limit4)
	{
		for (int i = 0; i < divRoundUp(numSectors, NumDirect); i++)
		{
			FileHeader *subhdr = new FileHeader;
			subhdr->FetchFrom(dataSectors[i]);
			subhdr->Deallocate(freeMap);
		}
	}
	// 2nd level
	else if (numBytes > Limit3)
	{
		for (int i = 0; i < divRoundUp(numSectors, NumDirect); i++)
		{
			FileHeader *subhdr = new FileHeader;
			subhdr->FetchFrom(dataSectors[i]);
			subhdr->Deallocate(freeMap);
		}
	}
	// 3rd level
	else if (numBytes > Limit2)
	{
		for (int i = 0; i < divRoundUp(numSectors, NumDirect); i++)
		{
			FileHeader *subhdr = new FileHeader;

			subhdr->FetchFrom(dataSectors[i]);
			subhdr->Deallocate(freeMap);
		}
	}
	// 4th level
	else
	{
		for (int i = 0; i < numSectors; i++)
		{
			ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
			freeMap->Clear((int)dataSectors[i]);
		}
	}
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
	kernel->synchDisk->ReadSector(sector, (char *)this);

	/*
		MP4 Hint:
		After you add some in-core informations, you will need to rebuild the header's structure
	*/
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
	kernel->synchDisk->WriteSector(sector, (char *)this);

	/*
		MP4 Hint:
		After you add some in-core informations, you may not want to write all fields into disk.
		Use this instead:
		char buf[SectorSize];
		memcpy(buf + offset, &dataToBeWritten, sizeof(dataToBeWritten));
		...
	*/
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset)
{
	// 1st level
	if (numBytes > Limit4)
	{
		int which = -1;
		which = divRoundDown(offset, Limit4);
		FileHeader *subhdr = new FileHeader;
		subhdr->FetchFrom(dataSectors[which]);
		subhdr->ByteToSector(offset - Limit4 * which);
	}
	// 2nd level
	else if (numBytes > Limit3)
	{
		int which = -1;
		which = divRoundDown(offset, Limit3);
		FileHeader *subhdr = new FileHeader;
		subhdr->FetchFrom(dataSectors[which]);
		subhdr->ByteToSector(offset - Limit3 * which);
	}
	// 3rd level
	else if (numBytes > Limit2)
	{
		int which = -1;
		which = divRoundDown(offset, Limit2);
		FileHeader *subhdr = new FileHeader;
		subhdr->FetchFrom(dataSectors[which]);
		subhdr->ByteToSector(offset - Limit2 * which);
	}
	// 4th level
	else
		return (dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
	printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
	// 4th level
	if (numBytes > Limit4)
	{
		for (int i = 0; i < numSectors / NumDirect; i++)
		{
			printf("first level hdr: %d\n", dataSectors[i]);
			OpenFile *openfile = new OpenFile(dataSectors[i]);
			FileHeader *subhdr = openfile->getHdr();
			subhdr->Print();
		}
	}
	// 3rd level
	else if (numBytes > Limit3)
	{
		for (int i = 0; i < numSectors / NumDirect; i++)
		{
			printf("first level hdr: %d\n", dataSectors[i]);
			OpenFile *openfile = new OpenFile(dataSectors[i]);
			FileHeader *subhdr = openfile->getHdr();
			subhdr->Print();
		}
	}
	// 2nd level hdr
	else if (numBytes > Limit2)
	{
		for (int i = 0; i < numSectors / NumDirect; i++)
		{
			printf("first level hdr: %d\n", dataSectors[i]);
			OpenFile *openfile = new OpenFile(dataSectors[i]);
			FileHeader *subhdr = openfile->getHdr();
			subhdr->Print();
		}
	}
	// 1st level header
	else
	{
		char *data = new char[SectorSize];
		int i, j, k;
		for (i = 0; i < numSectors; i++)
			printf("%d ", dataSectors[i]);
		printf("\nFile contents:\n");
		for (i = k = 0; i < numSectors; i++)
		{
			kernel->synchDisk->ReadSector(dataSectors[i], data);
			for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
			{
				if ('\040' <= data[j] && data[j] <= '\176')
					printf("%c", data[j]);
				else
					printf("\\%x", (unsigned char)data[j]);
			}
			printf("\n");
		}
		delete[] data;
	}
}
