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
    //printf("%d:  performed quicksort:", getpid()); printArray(A, n);
  } else { // else partition and recurse.

    int pipesToRead[levels - 1];
    int childLength[levels - 1];
    int leftLength = 0;

    int i;
    for(i = levels - 1; i >= 0; i--) {
      printf("%d: forloop start at level %d\n", getpid(), i);

      //set up pipe 
      int fd[2];
      int status = pipe(fd); if(status) perror("pipe error:");
      pipesToRead[i] = fd[0];

      //partition
      int pivot = partition(A, n);
      leftLength = pivot + 1;
      int rightLength = n - leftLength;

      //store length
      childLength[i] = rightLength;
      printf("%d: childLength[%d] assigned with %d\n", getpid(), i, rightLength);
      
      //fork
      int forkId = fork();
      if(!forkId) {
        // recurse
        int *output = recurse(&A[leftLength], rightLength, i);

        // send data back through pipe
        int nbytes = write(fd[1], output, rightLength * sizeof(int));
        printf("%d: sent %d elements to parent process\n", getpid(), (int) (nbytes/sizeof(int))); 
	printf("%d: child terminating\n", getpid());
        exit(0);

      } else {
        // parent debugging:
	//printf("%d: forked child with pid %d\n", getpid(), forkId);
      }

      n = leftLength;

    } // end for

    quickSort(A, leftLength);
    //printf("%d: performed quicksort:", getpid()); printArray(A, leftLength);

    int acc = leftLength;
    
    // wait for all children to exit
    //while (wait(NULL)) {
    //  if (errno == ECHILD) {
    //    break;
    //  }
    // }

    for(i = 0; i < levels; i++) {
      //combine pipes
      int nbytes = read(pipesToRead[i], &A[acc], childLength[i] * sizeof(int));  
      printf("%d: read %d elements from process on level %d\n", getpid(), (int) (nbytes/sizeof(int)), i);

      //increment acc
      acc += childLength[i];

    } // end for pipes

  } // endif

  return A;

}

// distributed quick sort using pipes
void quickPipe(int A[], int n, int p) {
  //printf("%d: start array:", getpid()); printArray(A, n);
  int levels = lg2(p);
  A = recurse(A, n, levels);
  //printArray(A, n);
} 


// distributed quick sort using sockets
void quickSocket(int A[], int n, int p) {
} //quickSocket()



// concurrent quick sort using pthreads 
void quickThread(int *pA, int pn, int p, enum WaitMechanismType pWaitMech) {
} //quickThread()

