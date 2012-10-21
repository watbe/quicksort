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

typedef int bool;
#define true 1
#define false 0

// distributed quick sort using pipes
void quickPipe(int A[], int n, int p) {

  //int fraction = p;
  
  bool hasParent = false;
  
  int childLength = 0;
  int leftLength = 0;
  int rightLength = 0;

  int forkId = 0;
  int fd_cp[2];
  
  int indicator;

  int levels = lg2(p);

  while(levels != 0) {

    // Partition into two
    int pivot = partition(A, n);

    // split off right side 
    leftLength = pivot + 1;
    rightLength = n - leftLength;  
    int right[rightLength];
    memmove(right, &A[leftLength], sizeof(int) * rightLength);

    // construct pipes
    int status = pipe(fd_cp); if (status != 0) perror("pipe error: ");

    // fork
    printf("forked a new process\n");
    forkId = fork();

    // if it's the child, set A[] to the right side, length
    // to the new length;
    if(!forkId) {
  
      // flag that it has a parent because forkId gets overwritten
      hasParent = true;
      
      //child
      n = rightLength;
      // make array smaller
      A = malloc(rightLength * sizeof(int));
      // copy into A
      memmove(A, right, rightLength * sizeof(int));
    } else {
      // parent needs new length to tell it how much to sort.
      // but array A can be the same (should maybe free()?).
      n = leftLength;
      childLength = rightLength;
    }

    levels--;

  }

  if(forkId && hasParent) {

    //parent are always children unless !hasParent
    quickSort(A, n);
    printf("performed quickSort as parent, awaiting merge\n");
    
    if(childLength > 0) {
      
      int child[childLength];
      int nbytes = read(fd_cp[0], child, childLength * sizeof(int));
      
      printf("read %d bytes from child process\n", nbytes);

    }

    waitpid(-1, &indicator, 0);

    // send data back through pipe
    int nbytes = write(fd_cp[1], A, leftLength * sizeof(int));
    printf("sent %d bytes to parent process\n", nbytes);

    
    // close pipes
    close(fd_cp[0]);
    close(fd_cp[1]);
    
    // terminate
    printf("terminating child...!\n");
    exit(0);

  } else if(hasParent) {
    //child 
    quickSort(A, n);
    printf("performed quickSort as leaf\n");
    
    // send data back through pipes
    int nbytes = write(fd_cp[1], A, leftLength * sizeof(int));
    printf("sent %d bytes to parent process\n", nbytes);

    // close pipes
    close(fd_cp[0]);
    close(fd_cp[1]);
    
    // terminate
    printf("terminating child...!\n");
    exit(0);

  } else {

    quickSort(A, n);
    printf("performed quickSort as root process, waiting on merge\n");

    if(childLength > 0) {
      
      int child[childLength];
      int nbytes = read(fd_cp[0], child, childLength * sizeof(int));
      
      printf("read %d bytes from child process\n", nbytes);

      waitpid(-1, &indicator, 0);

    }

    printf("root finished! \n");

  }

}

void notUsed(int A[], int n, int p) {
  //printArray(A, n);

  //int pivot = partition(A, n);

  //printArray(A, n);
 
  //printf("pivot: %d\n", pivot);

  //quickSort(left, leftLength);
  //quickSort(right, rightLength);

  //printf("left: "); printArray(left, leftLength);
  //printf("right: "); printArray(right, rightLength);
  
  //memmove(A, left, sizeof(int) * leftLength);
  //memmove(&A[leftLength], right, sizeof(int) * rightLength);
  
  //printf("output: "); printArray(A, n);

} 


// distributed quick sort using sockets
void quickSocket(int A[], int n, int p) {
} //quickSocket()



// concurrent quick sort using pthreads 
void quickThread(int *pA, int pn, int p, enum WaitMechanismType pWaitMech) {
} //quickThread()

