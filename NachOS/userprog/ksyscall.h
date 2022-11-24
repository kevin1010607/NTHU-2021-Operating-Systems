/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt()
{
	kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}

#ifdef FILESYS_STUB
int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}
#endif
int SysCreate(char *filename, int size)
{
	return kernel->interrupt->Create(filename, size);
}
int SysOpen(char *filename)
{
	return kernel->interrupt->Open(filename);
}
int SysWrite(char *buffer, int size, int ID)
{
	return kernel->interrupt->Write(buffer, size, ID);
}
int SysClose(int ID)
{
	return kernel->interrupt->Close(ID);
}
int SysRead(char *buffer, int size, int ID)
{
	return kernel->interrupt->Read(buffer, size, ID);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
