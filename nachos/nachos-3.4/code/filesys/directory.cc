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
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include <string.h>


#define rootSector 	1
#define NumDirEntries 		10

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
    tableSize = size;
    for (int i = 0; i < 2; i++) {
        table[i].inUse = true;
        table[i].sector = rootSector;
        table[i].type = 0;
    }
    strcpy(table[0].name, ".");
    strcpy(table[1].name, "..");
    for (int i = 2; i < tableSize; i++) {
	    table[i].inUse = FALSE;
    }
}

Directory::Directory(int size, int sector, int dad_sector) {
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < 2; i++) {
        table[i].inUse = true;
        table[i].type = 0;
    }
    table[0].sector = sector;
    table[1].sector = dad_sector;
    strcpy(table[0].name, ".");
    strcpy(table[1].name, "..");
    for (int i = 2; i < tableSize; i++) {
	    table[i].inUse = FALSE;
    }
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    //printf("I am finding %s\n", name);
    //List();
    bool abs_path = false;
    int str_p;
    int len = strlen(name);
    for (int i = 0; i < len; i++) {
        if (name[i] == '/') {
            abs_path = true;
            str_p = i;
            break;
        }
    }
    if (abs_path) {
        char *tmp_name = new char[len - str_p + 2];
        strcpy(tmp_name, name + str_p + 1);
        char *dir_name = new char[str_p + 2];
        strncpy(dir_name, name, str_p);
        int dir_idx = -1;
        for (int i = 0; i < tableSize; i++) {
            if (table[i].inUse && table[i].type == 0 && !strncmp(table[i].name, dir_name, FileNameMaxLen)) {
                dir_idx = i;
            }
        }
        OpenFile *next_dir = new OpenFile(table[dir_idx].sector);
        Directory *next_dir_file = new Directory(NumDirEntries);
        next_dir_file->FetchFrom(next_dir);
        int res_idx = next_dir_file->FindIndex(tmp_name);
        delete next_dir;
        delete next_dir_file;
        return res_idx;
    }
    else {
        for (int i = 0; i < tableSize; i++) {
            if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen)) {
                return i;
            }
        }
    }
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
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

bool
Directory::Add(char *name, int newSector, int type)
{ 
    if (FindIndex(name) != -1)
	return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            //strcpy(table[i].path, name); 
            strncpy(table[i].name, getRealName(name), FileNameMaxLen);
            table[i].sector = newSector;
            table[i].type = type;
            return TRUE;
	    }
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);
    //printf("remove i: %d\n", i);
    if (i == -1)
	    return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
    printf("\nIN DIRECTORY TABLE:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            printf("%s\n", table[i].name);
    printf("END\n\n");
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}
