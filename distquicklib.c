/* distquicklib: library of concurrent and distributed quicksort algorithms 
   for COMP2310 Assignment 2, 2012.

   Name: Wei-en (Wayne) Tsai 
   StudentId: U5027622

***Disclaimer***: (modify as appropriate)
  The work that I am submitting for this program is without significant
  contributions from others (excepting course staff).
*/

// uncomment when debugging. Example call: PRINTF(("x=%d\n", x));
//#define PRINTF(x) do { printf x; fflush(stdout); } while (0) 
#define PRINTF(x) /* use when not debugging */

#include <stdio.h>
#include <stdlib.h>  	/* malloc, free */
#include <strings.h> 	/* bcopy() */
#include <assert.h>  
#include <unistd.h>     /* fork(), pipe() */
#include <sys/types.h>
#include <sys/wait.h>   
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include <string.h>

#include "quicklib.h"
#include "distquicklib.h"

#include <errno.h>

typedef int bool;
#define true 1
#define false 0

int* recurse(int A[], int n, int levels) {
  // if it's the final level, then don't partition, just quicksort
  if(levels == 0) {
    quickSort(A, n);
  } else { // else partition and recurse.
    int pivot = partition(A, n);
    int leftLength = pivot + 1;
    int rightLength = n - pivot - 1;
    
    //set up pipe 1
    int fd_left[2];
    int status = pipe(fd_left); if(status) perror("pipe error:");
    int forkId = fork();
    if(!forkId) {
      int *output = recurse(A, leftLength, levels - 1);
      // send data back through pipe
      int nbytes = write(fd_left[1], output, leftLength * sizeof(int));
      printf("%d: sent %d elements to parent process\n", getpid(), (int) (nbytes/sizeof(int))); 
      exit(0);
    }

    //set up pipe 2
    int fd_right[2];
    status = pipe(fd_right); if(status) perror("pipe error:");
    forkId = fork();
    if(!forkId) {
      int *output = recurse(&A[leftLength], rightLength, levels - 1);
      // send data back through pipe
      int nbytes = write(fd_right[1], output, rightLength * sizeof(int));
      printf("%d: sent %d elements to parent process\n", getpid(), (int) (nbytes/sizeof(int))); 
      exit(0);
    }

    // wait for all children to exit
    while (wait(NULL)) {
      if (errno == ECHILD) {
        break;
      }
    }

    //combine pipes
    int nbytes = read(fd_left[0], A, leftLength * sizeof(int));  
    printf("%d: read %d elements from left process\n", getpid(), (int) (nbytes/sizeof(int)));
    nbytes = read(fd_right[0], &A[leftLength], rightLength * sizeof(int));  
    printf("%d: read %d elements from right process\n", getpid(), (int) (nbytes/sizeof(int)));

  }

  return A;

}

// distributed quick sort using pipes
void quickPipe(int A[], int n, int p) {
  int levels = lg2(p);
  A = recurse(A, n, levels);
} 


// distributed quick sort using sockets
void quickSocket(int A[], int n, int p) {
} //quickSocket()



// concurrent quick sort using pthreads 
void quickThread(int *pA, int pn, int p, enum WaitMechanismType pWaitMech) {
} //quickThread()

