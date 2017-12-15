

/*
        This file contains the functions that the forked children will call 
        they will take the input, collapse stars, replace newlines with spaces,
        and print the output.  This will be accomplished using memory mapping 
        buffers between each process.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> // kill
#include <signal.h> // kill
#include <sys/wait.h>
#include <unistd.h>
#include "buffer.h"

#define BUFFER_SIZE 256
#define LINE_SIZE 80
#define ERROR -1

// threads 2 and 3 need two buffers each so make structs for these
struct DoubleBuffer {
        Buffer input;
        Buffer output;
};

// function for taking input
void takeInput(void *outputBuffer){
        // type cast the buffer
        (Buffer)outputBuffer;

        // initialize variables used
        char c;

        // take input from stdin and deposit into the mapped file
        while(scanf("%c", &c) != EOF){
                deposit(outputBuffer, c);
        }

        // deposit EOF so next thread knows its done 
        deposit(outputBuffer, EOF);

        // exit the process
        exit(EXIT_SUCCESS);
}

// function for processing newline
void processNewline(void *dubBuffer){
        // create pointer for DoubleBuffer        
        struct DoubleBuffer *db = dubBuffer;

        // initialize variables used
        char c;

        // take input and remove newlines
        while((c = remoove(db->input)) != EOF){
                // replace the newline
                if(c == '\n'){
                        c = ' ';
                }

                // deposit the char in mapped memory
                deposit(db->output, c);
        }

        // deposit the EOF
        deposit(db->output, c);

        // exit the process
        exit(EXIT_SUCCESS);
}

// function for removing double star and replacing with ^
void processStar(void *dubBuffer){
        // create pointer for DoubleBuffer
        struct DoubleBuffer *db = dubBuffer;

        // initialize variables used
        char current;
        char previous = '\0';
        int starCollapse = 0;

        // take input and remove **
        do {
                // take input from memory mapped file
                current = remoove(db->input);                

                // remove the star and deposit into mapped file
                if(current == '*' && previous == '*'){
                        deposit(db->output, '^');
                        current = '\0';
                        starCollapse = 1;
                }
                else if(previous == '*'){
                        if(!starCollapse){
                                deposit(db->output, previous);
                        }
                        starCollapse = 0;
                }
                else{
                        if(previous != '\0'){
                                deposit(db->output, previous);
                        }
                        starCollapse = 0;
                }

                // assign previous char
                previous = current;
        } while(current != EOF);

        // deposit the EOF
        deposit(db->output, current);

        // exit the process
        exit(EXIT_SUCCESS);
}

// function for printing output
void printOutput(void *inputBuffer){
        // type cast the buffer
        (Buffer)inputBuffer;

        char line[LINE_SIZE + 1];
        char c;
        int charPos;        

        // loop to print output taken from mapped file
        while((c = remoove(inputBuffer)) != EOF){
                // put input in line
                if(charPos < LINE_SIZE){
                        line[charPos] = c;
                }

                // increment position
                charPos++;

                // if end of line has been reached, print
                if(charPos == LINE_SIZE){
                        line[charPos] = 0;
                        printf("%s\n", line);
                        charPos = 0;
                }
        }

        // exit the process
        exit(EXIT_SUCCESS);
}

// function to wait for the child processes to exit
void waitForChildren(pid_t* childpids){
        int status;
        while(ERROR < wait(&status)){
                if(!WIFEXITED(status)){
                        kill(childpids[0], SIGKILL);
                        kill(childpids[1], SIGKILL);
                        kill(childpids[2], SIGKILL);
                        kill(childpids[3], SIGKILL);
                        break;
                }
        }
}

// function to fork the process and return the childs pid
pid_t forkChild(void (*function)(void *), void* bufOrDubBuf){
        pid_t childpid;
        switch (childpid = fork()){
                case ERROR:
                        fprintf(stderr, "fork error\n");
                        exit(EXIT_FAILURE);
                case 0:
                        (*function)(bufOrDubBuf);
                default:
                        return childpid;
        }
}

// main function that creates children and buffers for between them
int main(){
        // make buffers for the threads
        // this is where the memory mapping occurs, the buffer pointers
        // will now point to shared memory
        Buffer buffer0 = createBuffer(BUFFER_SIZE, "empty000", "full000");
        Buffer buffer1 = createBuffer(BUFFER_SIZE, "empty001", "full001");
        Buffer buffer2 = createBuffer(BUFFER_SIZE, "empty002", "full002");

        struct DoubleBuffer *dubBuf0 = malloc(sizeof(struct DoubleBuffer));
        dubBuf0->input = buffer0;
        dubBuf0->output = buffer1;
        struct DoubleBuffer *dubBuf1 = malloc(sizeof(struct DoubleBuffer));
        dubBuf1->input = buffer1;
        dubBuf1->output = buffer2;

        // fork children
        pid_t childpids[3];
        childpids[0] = forkChild(takeInput, buffer0);
        childpids[1] = forkChild(processNewline, dubBuf0);
        childpids[2] = forkChild(processStar, dubBuf1);
        childpids[3] = forkChild(printOutput, buffer2);

        // wait for children to finish
        waitForChildren(childpids);

        // cleanup the buffers
        free(dubBuf0);
        free(dubBuf1);
        deleteBuffer(buffer0, "empty000", "full000");
        deleteBuffer(buffer1, "empty001", "full001");
        deleteBuffer(buffer2, "empty002", "full002");

        return 0;
}

