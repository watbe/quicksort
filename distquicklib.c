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


int recursive_partition(int A[], int n, int p, int *segments, int *segIndex, int leftLength) {
  
  if(!p) {printf("=====\n"); return 0;}

  printf("segIndex: %d partition request for:", *segIndex); printArray(A, n);
  int pivot = partition(A, n);
  
  printf("partitioned left:"); printArray(A, pivot+1);
  printf("partitioned right:"); printArray(&A[pivot+1], n-pivot-1);
  
  segments[*segIndex] = leftLength + pivot + 1;
  *segIndex = *segIndex+1;
  printf("pivot at: %d, added length: %d, at segIndex: %d\n", pivot+1, leftLength, *segIndex);

  recursive_partition(A, pivot+1, p-1, segments, segIndex, leftLength);

  recursive_partition(&A[pivot+1], n-pivot-1, p-1, segments, segIndex, pivot + 1);

  return n;

}

// distributed quick sort using pipes
void quickPipe(int A[], int n, int p) {

  printf("original array:"); printArray(A, n);
  //calculate partitions and partition
  int levels = lg2(p);

  // segments are the index of each child, index of 0 for root is assumed
  int segments[p];
  segments[p-1] = n;
  int seg_index = 0;
  int *seg_ptr = &seg_index;
  *seg_ptr = 0;

  recursive_partition(A, n, levels, segments, seg_ptr, 0);
  
  // sort segments (using provided quicksort)...
  quickSort(segments, p);

  printf("partition result:"); printArray(A, n);

  printf("segments:"); printArray(segments, p);
  
  // set up pipes
  int pipeReadArray[p - 1]; // read-write pairs
  int pipeWriteArray[p - 1]; 
  
  int i;
  for(i = 0; i < p - 1; i++) {
    int fd_child_to_parent[2];
    int status = pipe(fd_child_to_parent); if (status) perror("pipe error:");
    pipeReadArray[i] = fd_child_to_parent[0];
    pipeWriteArray[i] = fd_child_to_parent[1];
    printf("piped! %d, %d\n", fd_child_to_parent[0], fd_child_to_parent[1]);
  }

  printf("pipeReadArray:"); printArray(pipeReadArray, p-1);
  printf("pipeWriteArray:"); printArray(pipeWriteArray, p-1);

  for(i = 0; i < p - 1; i++) {
    if(!fork()) {
      //child sorts their segment
      int childStartIndex = segments[i];
      int childLength = segments[i+1] - childStartIndex;
      printf("childStartIndex: %d, childLength: %d\n", childStartIndex, childLength);
      
      quickSort(&A[childStartIndex], childLength);
      printf("child sorted: "); printArray(&A[childStartIndex], childLength);
      
      // send data back through pipe
      int nbytes = write(pipeWriteArray[i], &A[childStartIndex], childLength * sizeof(int));
      printf("%d: sent %d elements to parent process\n", getpid(), (int) (nbytes/sizeof(int))); 

      exit(0);  
    }
  }
  
  quickSort(A, segments[0]);  
  printf("root sorted: "); printArray(A, segments[0]);
 
  // wait for all children to exit
  while (wait(NULL)) {
    if (errno == ECHILD) {
      break;
    }
  }
  
  for(i = 0; i < p - 1; i++) {
    int nbytes = read(pipeReadArray[i], &A[segments[i]], (segments[i+1] - segments[i]) * sizeof(int));  
    printf("%d: read %d elements from child process\n", getpid(), (int) (nbytes/sizeof(int)));
  }

  printf("final result: "); printArray(A, n);

} 


// distributed quick sort using sockets
void quickSocket(int A[], int n, int p) {
} //quickSocket()



// concurrent quick sort using pthreads 
void quickThread(int *pA, int pn, int p, enum WaitMechanismType pWaitMech) {
} //quickThread()

