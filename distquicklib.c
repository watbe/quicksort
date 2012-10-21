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


// distributed quick sort using pipes
void quickPipe(int A[], int n, int p) {

  while(p >= 2) {
    
  }
  //printArray(A, n);

  int pivot = partition(A, n);

  //printArray(A, n);
 
  printf("pivot: %d\n", pivot);

  int leftLength = pivot + 1;
  int rightLength = n - leftLength;  

  int left[leftLength];
  int right[rightLength];

  memmove(left, A, sizeof(int) * leftLength);
  memmove(right, &A[leftLength], sizeof(int) * rightLength);

  quickSort(left, leftLength);
  quickSort(right, rightLength);

  //printf("left: "); printArray(left, leftLength);
  //printf("right: "); printArray(right, rightLength);
  
  memmove(A, left, sizeof(int) * leftLength);
  memmove(&A[leftLength], right, sizeof(int) * rightLength);
  
  //printf("output: "); printArray(A, n);

} 


// distributed quick sort using sockets
void quickSocket(int A[], int n, int p) {
} //quickSocket()



// concurrent quick sort using pthreads 
void quickThread(int *pA, int pn, int p, enum WaitMechanismType pWaitMech) {
} //quickThread()

