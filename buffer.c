

/*
* This file contains fucntions for manipulating data in a Buffer struct.
* This includes deposit, remoove, createBuffer, and deleteBuffer.
* The struct is also defined in this file and named Buffer.
*/

#include "buffer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/mman.h> // mmap, munmap
#include <unistd.h> // sleep
#include <sys/types.h> // kill
#include <signal.h> // kill
#include <sys/stat.h>
#include <fcntl.h>

#define ERROR -1

struct Buffer{
        sem_t* empty;
        sem_t* full; 
        int size;
        int nextIn;
        int nextOut;
        char MSG[];
};

// deposit a char into the given buffer
void deposit(Buffer buffer, char c){
        // down on empty buffer from lectures
        if (sem_wait(buffer->empty) == ERROR){
                fprintf(stderr, "error with down on empty semaphore\n");
                exit(EXIT_FAILURE);
        }

        // assert awaited condition - at least one empty buffer available
        int sem_val;
        if(sem_getvalue(buffer->empty, &sem_val)){
                fprintf(stderr, "error with sem_getvalue\n");
        }
        assert(sem_val >= 0);

        buffer->MSG[buffer->nextIn] = c;
        buffer->nextIn = (buffer->nextIn + 1) % buffer->size;

        // assert synchronization condition - at least one full buffer
        if(sem_getvalue(buffer->full, &sem_val)){
                fprintf(stderr, "error with sem_getvalue\n");
        }
        assert(sem_val < buffer->size);

        // up on full buffer from lectures
        if (sem_post(buffer->full) == ERROR){
                fprintf(stderr, "error with up on full semaphore\n");
                exit(EXIT_FAILURE);
        }
}

//remove a char from the buffer and return that char
char remoove(Buffer buffer){
        char c;

        // down on full buffer from lecture
        if (sem_wait(buffer->full) == ERROR){
                fprintf(stderr, "error with down on full semaphore\n");
                exit(EXIT_FAILURE);
        }

        // // assert awaited condition - at least one full buffer available
        int sem_val;
        if(sem_getvalue(buffer->full, &sem_val)){
                fprintf(stderr, "error with sem_getvalue\n");
        }
        assert(sem_val >= 0);

        c = buffer->MSG[buffer->nextOut];
        buffer->nextOut = (buffer->nextOut + 1) % buffer->size;

        // assert synchronization condition - at least one empty buffer
        if(sem_getvalue(buffer->empty, &sem_val)){
               fprintf(stderr, "error with sem_getvalue\n");
        }
        assert(sem_val < buffer->size);

        // up on empty buffer from lectures
        if (sem_post(buffer->empty) == ERROR){
                fprintf(stderr, "error with up on empty semaphore\n");
                exit(EXIT_FAILURE);
        }

        return c;
}

// create and place buffer on heap
Buffer createBuffer(int n, char* nameEmpty, char* nameFull){
        // neccessary arguments for mmap
        void* addr = 0;
        int fileSize = sizeof(struct Buffer) + ((n + 1) * sizeof(char));
        int protections = PROT_READ|PROT_WRITE;
        int flags = MAP_SHARED|MAP_ANONYMOUS;
        int fd = -1;
        off_t offset = 0;

        // create memory map
        Buffer buffer = mmap(addr, fileSize, protections, flags, fd, offset);

        if ((void *) ERROR == buffer){
                fprintf(stderr, "error with mmap\n");
                exit(EXIT_FAILURE);
        }

        // inialize the semaphores and other buffer variables
        buffer->empty = sem_open(nameEmpty, O_CREAT, S_IREAD | S_IWRITE, n);
        if(buffer->empty == SEM_FAILED){
                fprintf(stderr, "could not open semaphore\n");
                exit(EXIT_FAILURE);
        }
        buffer->full = sem_open(nameFull, O_CREAT, S_IREAD | S_IWRITE, 0);
        if(buffer->full == SEM_FAILED){
                fprintf(stderr, "could not open semaphore\n");
                exit(EXIT_FAILURE);
        } 

        buffer->size = n;
        buffer->nextIn = 0;
        buffer->nextOut = 0;
}

// clean up the memory map 
void deleteBuffer(Buffer buffer, char* nameEmpty, char* nameFull){
        int fileSize = sizeof(struct Buffer) + ((buffer->size + 1) * sizeof(char));

        // close semaphores
        if (sem_unlink(nameEmpty) == ERROR){
                fprintf(stderr, "error closing semaphore\n");
                exit(EXIT_FAILURE);
        }
        if (sem_unlink(nameFull) == ERROR){
                fprintf(stderr, "error closing semaphore\n");
                exit(EXIT_FAILURE);
        }

        // delete memory map
        if (ERROR == munmap(buffer, fileSize)){
                fprintf(stderr, "error deleting mmap\n");
                exit(EXIT_FAILURE);
        }
}

