// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include <string.h>
#include <stdio.h>

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = Thread::createThread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

void
ThreadTest2() {
    DEBUG('t', "Entering ThreadTest2");

    Thread *t[129];
    for (int i = 0; i < 128; i++) {
        char thread_name[20];
        sprintf(thread_name, "forked thread %d", i + 1);
        t[i] = Thread::createThread(thread_name);
        if (t[i] != NULL) {
            printf("i am thread %d\n", t[i]->getTid());
            t[i]->Fork(SimpleThread, i + 1);
        }
    }
    SimpleThread(0);
}

void
ThreadTest3() {
    DEBUG('t', "Entering ThreadTest2");
    /*
    Thread *t[3];
    for (int i = 0; i < 3; i++) {
        char thread_name[20];
        sprintf(thread_name, "forked thread %d", i + 1);
        t[i] = Thread::createThread(thread_name);
        if (t[i] != NULL) {
            //printf("i am thread %d\n", t[i]->getTid());
            t[i]->setPri(3 - i);
            t[i]->Fork(SimpleThread, i + 1);
        }
    }*/
    Thread *t1 = Thread::createThread("thread1");
    Thread *t2 = Thread::createThread("thread2");
    Thread *t3 = Thread::createThread("thread3");
    Thread *t4 = Thread::createThread("thread4");
    t1->setPri(3);
    t2->setPri(1);
    t3->setPri(2);
    t4->setPri(0);
    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    t3->Fork(SimpleThread, 3);
    t4->Fork(SimpleThread, 4);
    currentThread->Yield();
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
    Thread::ts();
	break;
    case 2:
    ThreadTest2();
    break;
    case 3:
    ThreadTest3();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}
