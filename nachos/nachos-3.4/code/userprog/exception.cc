// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "machine.h"
#include "translate.h"
#include <stdio.h>
#include "openfile.h"
#include "bitmap.h"
#include "synchdisk.h"
#include "filesys.h"
#include "filehdr.h"
#include "openfile.h"

#define LRU

void exec_func(int name) {
    char *filename = (char*) name;
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    printf("Exec thread is running...\n");
    machine->Run();			// jump to the user progam
}

class ThreadState {
    public:
    int pc;
    AddrSpace* space;
};

void fork_func(int s) {
    ThreadState* state = (ThreadState*) s;
    AddrSpace* tmp = state->space;
    AddrSpace* space = new AddrSpace(*tmp);
    currentThread->space = space;

    int cur_pc = state->pc;
    machine->WriteRegister(PCReg, cur_pc);
    machine->WriteRegister(NextPCReg, cur_pc + 4);

    currentThread->SaveUserState();
    printf("Prepare to run the fork thread\n");
    machine->Run();
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    }

    else if ((which == SyscallException) && (type == SC_Create)) {
        int name_pos = machine->ReadRegister(4);
        int v;
        int name_len = 0;
        char name[name_len + 1];
        int i = 0;
        while (1) {
            machine->ReadMem(name_pos, 1, &v);
            name_pos++;
            name_len++;
            if (v == 0) break;
            else {
                name[i] = (char)v;
                i++;
            }
        }
        name[name_len] = '\0';
        printf("Creating file: %s\n", name);
        fileSystem->Create(name, 0);
        printf("Creating success!\n");
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Open)) {
        int name_pos = machine->ReadRegister(4);
        int v;
        int name_len = 0;
        char name[name_len + 1];
        int i = 0;
        while (1) {
            machine->ReadMem(name_pos, 1, &v);
            name_pos++;
            name_len++;
            if (v == 0) break;
            else {
                name[i] = (char)v;
                i++;
            }
        }
        name[name_len] = '\0';
        printf("Opening file: %s\n", name);
        OpenFile *openfile = fileSystem->Open(name);
        if (openfile == NULL) {
            printf("Open file failed, file name: %s\n", name);
        }
        else {
            machine->WriteRegister(2, (int)openfile);
            printf("Open success!\n");
        }
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Read)) {
        int buff = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int id = machine->ReadRegister(6);

        OpenFile* tmpfile = (OpenFile*) id;
        if (tmpfile == NULL) {
            printf("File not exist\n");
        }
        else {
            printf("Begin reading...\n");
            char tmp_buffer[size];
            int tmp, i;
            tmpfile->Read(tmp_buffer, size);
            tmp_buffer[size] = '\0';
            for (i = 0; i < size; i++) {
                tmp = (int)tmp_buffer[i];
                machine->WriteMem(buff + i, 1, tmp);
            }
            machine->WriteRegister(2, i);
            printf("Read success. Content is %s\n", tmp_buffer);
        }
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Write)) {
        int buff = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        int id = machine->ReadRegister(6);

        char tmp_buffer[size];
        int tmp;
        for (int i = 0; i < size; i++) {
            machine->ReadMem(buff + i, 1, &tmp);
            tmp_buffer[i] = (char)tmp;
        }

        OpenFile* tmpfile = (OpenFile*) id;
        
        if (tmpfile == NULL) {
            printf("File not exist\n");
        }
        else {
            printf("Begin writing...\n");
            tmpfile->Write(tmp_buffer, size);
            printf("Write success. Content is %s\n", tmp_buffer);
        }
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Close)) {
        int file = machine->ReadRegister(4);
        OpenFile* tmpfile = (OpenFile*) file;
        if (tmpfile == NULL) {
            printf("Cannot close file\n");
        }
        else {
            delete tmpfile;
            printf("Close file success!\n");
        }
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Exec)) {
        int name_pos = machine->ReadRegister(4);
        int v;
        int name_len = 0;
        char name[name_len + 1];
        int i = 0;
        while (1) {
            machine->ReadMem(name_pos, 1, &v);
            //printf("hhhh %c\n", (char)v);
            name_pos++;
            //name_len++;
            if ((char)v == '\0') break;
            else {
                name[i] = (char)v;
                i++;
            }
        }
        name[i] = '\0';
        //printf("name: %s\n", name);

        Thread* new_thread = new Thread("new thread");
        for (i = 0; i < 10; i++) {
            if (currentThread->child[i] == NULL) {
                currentThread->child[i] = new_thread;
                machine->WriteRegister(2, (int)new_thread);
                break;
            }
            if (i == 9 && currentThread->child != NULL) {
                printf("Exec fail!\n");
                machine->PCAdvance();
                return;
            }
        }
        new_thread->father = currentThread;
        new_thread->Fork(exec_func, (int)name);
        currentThread->Yield();
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Fork)) {
        int cur_pc = machine->ReadRegister(4);
        ThreadState* state = new ThreadState;
        state->pc = cur_pc;
        state->space = currentThread->space;

        Thread* new_thread = new Thread("new thread");
        for (int i = 0; i < 10; i++) {
            if (currentThread->child[i] == NULL) {
                currentThread->child[i] = new_thread;
                machine->WriteRegister(2, (int)new_thread);
                break;
            }
            if (i == 9 && currentThread->child != NULL) {
                printf("Fork fail!\n");
                machine->PCAdvance();
                return;
            }
        }
        new_thread->father = currentThread;

        printf("Fork a new thread\n");
        new_thread->Fork(fork_func, (int)state);
        currentThread->Yield();
        printf("I am main thread. I am back!\n");
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Yield)) {
        machine->PCAdvance();
        printf("I am yield\n");
        currentThread->Yield();
    }

    else if ((which == SyscallException) && (type == SC_Join)) {
        int id = machine->ReadRegister(4);
        int i;
        bool found;

        Thread* child = (Thread*) id;
        for (i = 0; i < 10; i++) {
            if (currentThread->child[i] == child) {
                found = true;
                break;
            }
        }
        if (!found) {
            printf("Cannot find thread\n");
            machine->PCAdvance();
            return;
        }
        while (currentThread->child[i] != NULL) {
            //printf("I am waiting for my child %d\n", i);
            currentThread->Yield();
        }
        printf("My child is finished\n");
        machine->PCAdvance();
    }

    else if ((which == SyscallException) && (type == SC_Exit)) {
        /*
        for (int i = 0; i < machine->pageTableSize; i++) {
            if (machine->pageTable[i].valid == TRUE) {
                int idx = machine->pageTable[i].physicalPage;
                machine->bitmap->clear(idx);
            }
        }*/

        //printf("TLB hits: %d\n", machine->TLBhit);
        //printf("TLB miss: %d\n", machine->TLBmiss);
        //float rate = (float)machine->TLBhit / (machine->TLBmiss + machine->TLBhit);
        //printf("TLB hit rate: %f\n", rate);
        //machine->end = 1;
        if (currentThread->father == currentThread) {
            machine->PCAdvance();
        }
        else {
            Thread* father = currentThread->father;
            for (int i = 0; i < 10; i++) {
                if (father->child[i] == currentThread) {
                    //printf("ssssssssssssss %d\n", i);
                    father->child[i] = NULL;
                    break;
                }
            }
            currentThread->Finish();
        }
        //interrupt->Halt();
    }

    else if (which == PageFaultException) {
        int method = 0;
        if (machine->tlb == NULL) {
            OpenFile *openfile = fileSystem->Open("virtual_memory");
            if (openfile == NULL) ASSERT(false);
            int virtAddr = machine->registers[BadVAddrReg];
            unsigned int vpn = (unsigned) virtAddr / PageSize;
            unsigned int offset = (unsigned) virtAddr % PageSize;
            int pos = machine->bitmap->find();

            if (pos == -1) {
                pos = 0;
                for (int j = 0; j < machine->pageTableSize; j++) {
                    if (machine->pageTable[j].physicalPage == 0) {
                        if (machine->pageTable[j].dirty == TRUE) {
                            openfile->WriteAt(&(machine->mainMemory[pos * PageSize]),
                                PageSize, machine->pageTable[j].virtualPage * PageSize);
                            machine->pageTable[j].valid = FALSE;
                            break;
                        }
                    }
                }
            }
            //printf("PageFault vpn %d pos %d\n", vpn, pos);
            openfile->ReadAt(&(machine->mainMemory[pos * PageSize]), PageSize, vpn * PageSize);
            machine->pageTable[pos].valid = TRUE;
            machine->pageTable[pos].virtualPage = vpn;
            //machine->pageTable[vpn].physicalPage = pos;
            machine->pageTable[pos].use = FALSE;
            machine->pageTable[pos].dirty = FALSE;
            machine->pageTable[pos].readOnly = FALSE;
            delete openfile;
        }
        else {
            TranslationEntry *entry, *tlb;
            int virtAddr = machine->registers[BadVAddrReg];
            unsigned int vpn = (unsigned) virtAddr / PageSize;
            unsigned int offset = (unsigned) virtAddr % PageSize;
            entry = &machine->pageTable[vpn];
            tlb = machine->tlb;
            bool flag = false;

            for (int i = 0; i < TLBSize; i++) {
                if (tlb[i].valid) {
                    tlb[i].cnt++;
                }
            }

            for (int i = 0; i < TLBSize; i++) {
                if (!tlb[i].valid) {
                    tlb[i].valid = true;
                    tlb[i].virtualPage = entry->virtualPage;
                    tlb[i].physicalPage = entry->physicalPage;
                    tlb[i].readOnly = entry->readOnly;
                    tlb[i].use = false;
                    tlb[i].dirty = false;
                    tlb[i].cnt = 0;

                    flag = true;
                    break;
                }
            }
            if (!flag) {
                int maxcnt = 0;
                int maxidx = -1;
                for (int i = 0; i < TLBSize; i++) {
                    if (tlb[i].cnt > maxcnt) {
                        maxcnt = tlb[i].cnt;
                        maxidx = i;
                    }
                }
                tlb[maxidx].virtualPage = entry->virtualPage;
                tlb[maxidx].physicalPage = entry->physicalPage;
                tlb[maxidx].readOnly = entry->readOnly;
                tlb[maxidx].use = false;
                tlb[maxidx].dirty = false;
                tlb[maxidx].cnt = 0;
            }
        }
    }
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
