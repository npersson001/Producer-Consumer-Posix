

#ifndef BUFFER_H
#define BUFFER_H

// A Buffer struct to hold the array of input and output
typedef struct Buffer *Buffer;

// A function to deposit a char at end of the buffer
void deposit(Buffer buffer, char c);

// A fucntion to remove a value from front of the buffer
char remoove(Buffer buffer);

// A function to initialize the Buffer struct with size n
Buffer createBuffer(int n, char* e, char* f);

// A function to clean up the memory and close semaphores 
void deleteBuffer(Buffer buffer, char* e, char* f);

#endif /** BUFFER_H */

