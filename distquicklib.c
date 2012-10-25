/* distquicklib: library of concurrent and distributed quicksort algorithms 
   for COMP2310 Assignment 2, 2012.

   Name: Wei-en (Wayne) Tsai 
   StudentId: U5027622

***Disclaimer***: (modify as appropriate)
  The work that I am submitting for this program is without significant
  contributions from others (excepting course staff).
*/

// uncomment when debugging. Example call: PRINTF((("x=%d\n", x));
#define PRINTF(x) do { printf x; fflush(stdout); } while (0) 
//#define PRINTF((x) /* use when not debugging */

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

#define MAXBUF 2048

int* recurse(int A[], int n, int levels) {
  int length = n;

  // if it's the final level, then don't partition, just quicksort
  if(levels == 0) {
    quickSort(A, n);
    //PRINTF(("%d:  performed quicksort:", getpid())); printArray(A, n);
  } else { // else partition and recurse.

    int pipesToRead[levels - 1];
    int childLength[levels - 1];
    int leftLength = 0;

    int i;
    for(i = levels - 1; i >= 0; i--) {
      PRINTF(("%d: forloop start at level %d\n", getpid(), i));

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
      PRINTF(("%d: childLength[%d] assigned with %d\n", getpid(), i, rightLength));
      
      //fork
      int forkId = fork();
      if(!forkId) {
        
	// recurse
        int *output = recurse(&A[leftLength], rightLength, i);

        // send data back through pipe
        int nbytes = write(fd[1], output, rightLength * sizeof(int));
	close(fd[1]);
	
	// assert that we wrote all bytes
	assert (nbytes == rightLength * sizeof(int));
        PRINTF(("%d: sent %d elements to parent process\n", getpid(), (int) (nbytes/sizeof(int))));
	PRINTF(("%d: child terminating\n", getpid()));
        exit(0);

      } else {
        // parent debugging:
	//PRINTF(("%d: forked child with pid %d\n", getpid(), forkId));
      }

      n = leftLength;

    } // end for

    quickSort(A, leftLength);
    //PRINTF(("%d: performed quicksort:", getpid()); printArray(A, leftLength));

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
      close(pipesToRead[i]);
      // assert that we recieve the amount expected
      assert(nbytes == childLength[i] * sizeof(int));
      PRINTF(("%d: read %d elements from process on level %d\n", getpid(), (int) (nbytes/sizeof(int)), i));

      //increment acc
      acc += childLength[i];

    } // end for pipes

    // ensure that all elements have been quicksorted
    assert(acc == length);

  } // endif

  return A;

}

// distributed quick sort using pipes
void quickPipe(int A[], int n, int p) {
  //PRINTF(("%d: start array:", getpid()); printArray(A, n);
  int levels = lg2(p);
  A = recurse(A, n, levels);
  //printArray(A, n);
} //quickPipe() 

int* recurseSockets(int A[], int n, int levels) {
  int length = n;

  // if it's the final level, then don't partition, just quicksort
  if(levels == 0) {
    quickSort(A, n);
    //PRINTF(("%d:  performed quicksort:", getpid())); printArray(A, n);
  } else { // else partition and recurseSockets.

    int socketsToRead[levels - 1];
    int childLength[levels - 1];
    int leftLength = 0;

    socklen_t namelen;
    struct sockaddr_in client, server; /* address info;
                                          see /usr/include/linux/in.h */

    int i;
    for(i = levels - 1; i >= 0; i--) {

      PRINTF(("%d: forloop start at level %d\n", getpid(), i));
      
      int sock_1;                        /* one socket descriptor */
    
      /* ----Create TCP/IP socket---- */
      sock_1 = socket(AF_INET, SOCK_STREAM, 0);
      if (sock_1 == -1) {
        perror("socket() Socket was not created");
        exit(-1);
      }
      printf("%d: Socket created successfully.\n", getpid());
    
      /* ----Address information for use with bind---- */
      server.sin_family = AF_INET;         /* it is an IP address */
      server.sin_port   = 0;               /* use O/S defined port number */
      server.sin_addr.s_addr = INADDR_ANY; /* use any interface on this host*/
    
      /* ----Bind socket to address and port---- */
      if (bind(sock_1, (struct sockaddr *) &server, sizeof(server))) {
        perror("Server bind error");
        exit(-1);
      }
    
      /* ----Find out what port number was assigned---- */
      namelen = sizeof(server);
      if (getsockname(sock_1, (struct sockaddr *) &server, &namelen)) {
        perror("Server get port number");
        exit(-1);
      }
      printf("The assigned server port number is %d\n", ntohs(server.sin_port));
      
      int portNumber = ntohs(server.sin_port);
    
      socketsToRead[i] = sock_1;

      //partition
      int pivot = partition(A, n);
      leftLength = pivot + 1;
      int rightLength = n - leftLength;

      //store length
      childLength[i] = rightLength;
      PRINTF(("%d: childLength[%d] assigned with %d\n", getpid(), i, rightLength));
      
      //fork
      int forkId = fork();
      if(!forkId) {
        
	      // recurseSockets
        int *output = recurseSockets(&A[leftLength], rightLength, i);

        PRINTF(("%d: trying to send data to parent\n", getpid()));
      
        int sockConnection;                /* client socket */
        struct sockaddr_in server; /* server address */
        int nbytes;
      
        /* ----Create TCP/IP socket---- */
        sockConnection = socket(AF_INET, SOCK_STREAM, 0);
        if (sockConnection < 0) {
          perror("Client socket creation");
          exit(-1);
        }
        printf("Client socket created\n");
      
        /* ----Address and port information for server---- */
        server.sin_family      = AF_INET;
        server.sin_port        = htons(portNumber);
        server.sin_addr.s_addr = INADDR_ANY; /* use any interface on this host*/
      
        if (connect(sockConnection, (struct sockaddr *) &server, sizeof(server))) {
          perror("Client connection failure");
          exit(-1);
        }

        /* ----Send the message---- */
        nbytes = send(sockConnection, output, rightLength * sizeof(int), 0);
        if (nbytes < 0) {
          perror("Client failed to send data");
          exit(-1);
        }
      
        /* ----Close socket and terminate---- */
        close(sockConnection);

        // assert that we wrote all bytes
        assert (nbytes == rightLength * sizeof(int));
        PRINTF(("%d: sent %d elements to parent process\n", getpid(), (int) (nbytes/sizeof(int))));
	      PRINTF(("%d: child terminating\n", getpid()));
        exit(0);

      } else {
        // parent debugging:
	      //PRINTF(("%d: forked child with pid %d\n", getpid(), forkId));
      }

      n = leftLength;

    } // end for
    
    //listen on all sockets
    for(i = 0; i < levels; i++) {
      /* ----Set queue limits on socket---- */
      if (listen(socketsToRead[i], 1)) {
        perror("Server listen error");
        exit(-1);
      }
    }

    quickSort(A, leftLength);
    //printf(("%d: performed quicksort:", getpid()); printarray(a, leftlength));

    int acc = leftLength;
    
    // wait for all children to exit
    //while (wait(null)) {
    //  if (errno == echild) {
    //    break;
    //  }
    // }
    
    for(i = 0; i < levels; i++) {

      /* ----now we block waiting for a connection---- */
      namelen = sizeof(client);

      int sock_connection = accept(socketsToRead[i], (struct sockaddr *) &client, &namelen);
      if (sock_connection < 0) {
        perror("server accept failed");
        exit(-1);
      }

      getsockname(sock_connection, (struct sockaddr *) &server, &namelen);
      PRINTF(("%d: server received connection from %s\n",
            getpid(), inet_ntoa(client.sin_addr)));
      
      /* ----wait to receive some data---- */
      int nbytes = recv(sock_connection, &A[acc], childLength[i] * sizeof(int), 0);
      if (nbytes < 0) {
        perror("server recv error");
        exit(-1);
      }
  
      /* ----close sockets and terminate---- */
      close(sock_connection);
      close(socketsToRead[i]);
  
      PRINTF(("%d: server finished.\n", getpid()));

      // assert that we recieve the amount expected
      assert(nbytes == childLength[i] * sizeof(int));
      PRINTF(("%d: read %d elements from process on level %d\n", getpid(), (int) (nbytes/sizeof(int)), i));

      //increment acc
      acc += childLength[i];

    } // end for pipes

    // ensure that all elements have been quicksorted
    assert(acc == length);

  } // endif

  return A;

}

// distributed quick sort using sockets
void quickSocket(int A[], int n, int p) {
  int levels = lg2(p);
  A = recurseSockets(A, n, levels);
} //quickSocket()

// declare function recurseThreads first for ease
void recurseThreads(int A[], int n, int levels); 

struct arg_struct {
    int *arg1;
    int arg2;
    int arg3;
};

void* threadMe(void *a) {
  struct arg_struct *args;
  args = (struct arg_struct *) a;
  int *startPtr = args->arg1;
  int lengthPtr = args->arg2;
  int levelsPtr = args->arg3;
  
  // recurse
  recurseThreads(startPtr, lengthPtr, levelsPtr);

  //TODO Assert	

  PRINTF(("%d: child terminating\n", getpid()));
  pthread_exit(0);
  
}

void recurseThreads(int A[], int n, int levels) {
  
  int length = n;

  // if it's the final level, then don't partition, just quicksort
  if(levels == 0) {
    quickSort(A, n);
    PRINTF(("%d: performed quicksort at level 0:", getpid())); printArray(A, n);
  } else { // else partition and recurse.

    pthread_t *threads = malloc((levels - 1) * sizeof(pthread_t)); 

    int childLength[levels - 1];
    int leftLength = 0;

    int i;
    for(i = levels - 1; i >= 0; i--) {
      PRINTF(("%d: forloop start at level %d\n", getpid(), i));

      //partition
      int pivot = partition(A, n);
      leftLength = pivot + 1;
      int rightLength = n - leftLength;

      //store length
      childLength[i] = rightLength;
      PRINTF(("%d: childLength[%d] assigned with %d\n", getpid(), i, rightLength));

      struct arg_struct args;
      args.arg1 = &A[leftLength];
      args.arg2 = rightLength;
      args.arg3 = i;

      pthread_create(&threads[i], NULL, threadMe, (void *) &args);

      n = leftLength;

    } // end for

    quickSort(A, leftLength);
    PRINTF(("%d: performed quicksort:", getpid()); printArray(A, leftLength));
    
    for(i = 0; i < levels; i++) {
      if(pthread_join(threads[i], NULL) != 0) printf("pthread_join error!\n");
    }

  } // endif

}

// concurrent quick sort using pthreads 
void quickThread(int *pA, int pn, int p, enum WaitMechanismType pWaitMech) {
  printf("%d: start array:",getpid()); printArray(pA, pn);
  int levels = lg2(p);
  recurseThreads(pA, pn, levels);
  printf("%d: end array:",getpid()); printArray(pA, pn);
} //quickThread()
