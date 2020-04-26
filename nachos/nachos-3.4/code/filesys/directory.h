// directory.h 
//	Data structures to manage a UNIX-like directory of file names.
// 
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and 
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 		20	// for simplicity, we assume 
					// file names are <= 9 characters long

// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.

class DirectoryEntry {
  public:
    bool inUse;				// Is this directory entry in use?
    int sector;				// Location on disk to find the 
					//   FileHeader for this file 
    char name[FileNameMaxLen + 1];	// Text name for file, with +1 for 
					// the trailing '\0'
    //int name_pos;
    //int name_len;
    int type; // 0 for folder, 1 for file
    //char path[100];
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk. 

class Directory {
  public:
    Directory(int size); 		// Initialize an empty directory
					// with space for "size" files
    Directory(int size, int sector, int dad_sector);
    ~Directory();			// De-allocate the directory

    void FetchFrom(OpenFile *file);  	// Init directory contents from disk
    void WriteBack(OpenFile *file);	// Write modifications to 
					// directory contents back to disk

    int Find(char *name);		// Find the sector number of the 
					// FileHeader for file: "name"

    bool Add(char *name, int newSector, int type);  // Add a file name into the directory

    bool Remove(char *name);		// Remove a file from the directory

    void List();			// Print the names of all the files
					//  in the directory
    void Print();			// Verbose print of the contents
					//  of the directory -- all the file
					//  names and their contents.
    static char* getAbsDir(char *name) {
        if (!absPath(name))
            ASSERT(FALSE);
        int len = strlen(name);
        int str_p;
        for (int i = len - 1; i > 0; i--) {
            if (name[i] == '/') {
                str_p = i;
                break;
            }
        }
        char *abs_path = new char[str_p + 1];
        //printf("str_p : %d\n", str_p);
        strncpy(abs_path, name, str_p);
        abs_path[str_p] = '\0';
        return abs_path;
    }

    static char* getRealName(char *name) {
        if (absPath(name)) {
          int len = strlen(name);
          int str_p;
          for (int i = len - 1; i > 0; i--) {
              if (name[i] == '/') {
                  str_p = i;
                  break;
              }
          }
          char *real_name = new char[len - str_p + 2];
          strcpy(real_name, name + str_p + 1);
          return real_name;
        }
        else {
          return name;
        }
    }

    static bool absPath(char *name) {
        int len = strlen(name);
        for (int i = len - 1; i > 0; i--) {
            if (name[i] == '/') {
                return true;
            }
        }
        return false;
    }

  private:
    int tableSize;			// Number of directory entries
    DirectoryEntry *table;		// Table of pairs: 
					// <file name, file header location> 

    int FindIndex(char *name);		// Find the index into the directory 
					//  table corresponding to "name"
};

#endif // DIRECTORY_H
