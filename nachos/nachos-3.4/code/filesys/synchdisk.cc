// synchdisk.cc 
//	Routines to synchronously access the disk.  The physical disk 
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"
#include "system.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because 
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
DiskRequestDone (int arg)
{
    SynchDisk* disk = (SynchDisk *)arg;

    disk->RequestDone();
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char* name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (int) this);
    for (int i = 0; i < NumSectors; i++) {
        mutex[i] = new Semaphore("file mutex", 1);
        reader_num[i] = 0;
        vis_num[i] = 0;
    }
    for (int i = 0; i < cache_size; i++) {
        cache[i].valid = false;
    }
    readerLock = new Lock("reader lock");
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;
    for (int i = 0; i < NumSectors; i++) {
        delete mutex[i];
    }
    delete readerLock;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    
    lock->Acquire();			// only one disk I/O at a time
    //disk->ReadRequest(sectorNumber, data);
    
    int find_pos = -1;
    for (int i = 0; i < cache_size; i++) {
        if (cache[i].valid && cache[i].sector == sectorNumber) {
            find_pos = i;
            break;
        }
    }
    if (find_pos == -1) {
        disk->ReadRequest(sectorNumber, data);
        int swap = -1;
        for (int i = 0; i < cache_size; i++) {
            if (cache[i].valid) {
                swap = i;
                break;
            }
        }
        if (swap == -1) {
            int lru_min = cache[0].cnt;
            int min_pos = 0;
            for (int i = 0; i < cache_size; i++) {
                if (cache[i].cnt < lru_min) {
                    lru_min = cache[i].cnt;
                    min_pos = i;
                }
            }
            swap = min_pos;
        }
        cache[swap].valid = true;
        cache[swap].dirty = 0;
        cache[swap].sector = sectorNumber;
        cache[swap].cnt = stats->totalTicks;
        bcopy(data, cache[swap].data, SectorSize);
    }
    else {
        //printf("Cache hit!\n");
        cache[find_pos].cnt = stats->totalTicks;
        bcopy(cache[find_pos].data, data, SectorSize);
        disk->HandleInterrupt();
    }
    
    semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
    for (int i = 0; i < cache_size; i++) {
        if (cache[i].sector == sectorNumber) {
            cache[i].valid = false;
        }
    }
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{ 
    semaphore->V();
}


void SynchDisk::ReaderIn(int sector) {
    readerLock->Acquire();
    reader_num[sector]++;
    if (reader_num[sector] == 1) {
        mutex[sector]->P();
    }
    //printf("%s ", currentThread->getName());
    //printf("Reader in! Count of Reader: %d\n", reader_num[sector]);
    readerLock->Release();
}

void SynchDisk::ReaderOut(int sector) {
    readerLock->Acquire();
    reader_num[sector]--;
    if (reader_num[sector] == 0) {
        mutex[sector]->V();
    }
    //printf("%s ", currentThread->getName());
    //printf("Reader out! Count of Reader: %d\n", reader_num[sector]);
    readerLock->Release();
}


void SynchDisk::WriteBegin(int sector) {
    mutex[sector]->P();
}

void SynchDisk::WriteDone(int sector) {
    mutex[sector]->V();
}