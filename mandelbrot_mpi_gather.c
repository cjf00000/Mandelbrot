/* Sequential Mandelbrot program */


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define MAX_THREADS 1000

int X_RESN, Y_RESN, nThreads;
int pointerSize = sizeof(char*);

typedef struct complextype
        {
        float real, imag;
        } Compl;  
	
Window          win;                            /* initialization for a window */
unsigned
int             width, height,                  /* window size */
		x, y,                           /* window position */
		border_width,                   /*border width in pixels */
		display_width, display_height,  /* size of screen */
		screen;                         /* which screen */

char            *window_name = "Mandelbrot Set", *display_name = NULL;
GC              gc;
unsigned
long            valuemask = 0;
XGCValues       values;
Display         *display;
XSizeHints      size_hints;
Pixmap          bitmap;
FILE            *fp, *fopen ();
char            str[100];

int sb_test[30000];

int min(int a, int b)
{
    return a<b ? a : b;
}

void XWindow_Init()
{
        XSetWindowAttributes attr[1];       
       
        /* connect to Xserver */

        if (  (display = XOpenDisplay (display_name)) == NULL ) {
           fprintf (stderr, "drawon: cannot connect to X server %s\n",
                                XDisplayName (display_name) );
        exit (-1);
        }
        
        /* get screen size */

        screen = DefaultScreen (display);
        display_width = DisplayWidth (display, screen);
        display_height = DisplayHeight (display, screen);

        /* set window size *///XFlush (display);
        //sleep (30);	

        width = X_RESN;
        height = Y_RESN;

        /* set window position */

        x = 0;
        y = 0;

        /* create opaque window */

        border_width = 4;
        win = XCreateSimpleWindow (display, RootWindow (display, screen),
                                x, y, width, height, border_width, 
                                BlackPixel (display, screen), WhitePixel (display, screen));

        size_hints.flags = USPosition|USSize;
        size_hints.x = x;
        size_hints.y = y;
        size_hints.width = width;
        size_hints.height = height;
        size_hints.min_width = 300;
        size_hints.min_height = 300;
        
        XSetNormalHints (display, win, &size_hints);
        XStoreName(display, win, window_name);

        /* create graphics context */

        gc = XCreateGC (display, win, valuemask, &values);

        XSetBackground (display, gc, WhitePixel (display, screen));
        XSetForeground (display, gc, BlackPixel (display, screen));
        XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);

        attr[0].backing_store = Always;
        attr[0].backing_planes = 1;
        attr[0].backing_pixel = BlackPixel(display, screen);

        XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);

        XMapWindow (display, win);
        XSync(display, 0);  
}

void XFinish()
{
    XFlush (display);
    sleep (30);	
}

void random_shuffle(int *begin, int *end);

void main (int argc, char **argv)
{
	// Initialize the MPI environment. 
	MPI_Init(NULL, NULL);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);    
    
	struct timespec start, finish;
	if (world_rank==0) clock_gettime(CLOCK_MONOTONIC, &start);
	
	if (world_rank==0) {
	    if ( argc < 3 ) {
		printf("Usage: %s X_RESN Y_RESN\n", argv[0]);
		exit(0);
	    }
	}	
	
	sscanf(argv[1], "%d", &X_RESN);
	sscanf(argv[2], "%d", &Y_RESN);	
	
#ifdef X_ENABLED	
	if (world_rank==0) XWindow_Init();	
#endif	
		
	int i, j, X2;
	int nSubProcesses = world_size - 1;	    
	//Make it divisible by nSubProcesses!
	//printf("X_RESN = %d\n", X_RESN);
	if (X_RESN % nSubProcesses != 0)
	    X2 = (X_RESN/nSubProcesses + 1) * nSubProcesses;
	else
	    X2 = X_RESN;
	int blockSize = X2 / nSubProcesses;
	//printf("nSubProcess = %d, blockSize = %d\n", nSubProcesses, blockSize);
	
	if (world_rank==0)				//Master Process
	{	    
	    int *tasks = malloc( sizeof(int) * X2 );
	    int *temp;
	    for (i=0; i<X2; ++i)
		tasks[i] = i;
	    random_shuffle(tasks, tasks + X2);
	    
	    //Send out tasks
	    for (i=0; i<nSubProcesses; ++i)
		MPI_Send(tasks+i*blockSize, blockSize, MPI_INT, i+1, 0, MPI_COMM_WORLD);
	    
	    //Receive points to draw
	    int iter;
	    temp = malloc( sizeof(int) * Y_RESN * 2 );
	    for (iter=0; iter<blockSize; ++iter)//This can reduce storage overhead. The order of loop can't be changed!			
	    {
		for (i=0; i<nSubProcesses; ++i)
		{
		    int nSize;		    
		    int taskLine = tasks[i*blockSize+iter];
		    if (taskLine>=X_RESN) continue;
		    // Receive number of points to draw
		    MPI_Recv(&nSize, 1, MPI_INT, i+1, taskLine, MPI_COMM_WORLD, MPI_STATUS_IGNORE);		    
		    // Receive points
		    MPI_Recv(temp, nSize, MPI_INT, i+1, taskLine+X2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
#ifdef X_ENABLED
		    //XWindow Draw		    
		    for (j=0; j<nSize; ++j) 
			XDrawPoint(display, win, gc, taskLine, temp[j]);
#endif
		}	       
	    }	 
	    free(temp);
	    free(tasks);	   	    
	}
	else						//Slave Process
	{	    
	    //Receive tasks from master
	    int *tasks = malloc( sizeof(int) * blockSize );
	    MPI_Recv(tasks, blockSize, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	   	    
	    
	    //Do calculations	    
	    int *temp = malloc( sizeof(int) * Y_RESN * 2 );		//Storage for points to draw
	    int x;
	    for (x=0; x<blockSize; ++x)
	    {		
		int nSize = 0;
		i = tasks[x];		
		if (i>=X_RESN) continue;
		Compl   z, c;
		float   lengthsq, fTemp;
		int k;
		for(j=0; j < Y_RESN; j++) {	   
		    z.real = z.imag = 0.0;
		    c.imag = ((float) j - Y_RESN/2)/(Y_RESN/4);
		    c.real = ((float) i - X_RESN/2)/(X_RESN/4);
		    k = 0;

		    do  {                                             // iterate for pixel color 
			fTemp = z.real*z.real - z.imag*z.imag + c.real;
			z.imag = 2.0*z.real*z.imag + c.imag;
			z.real = fTemp;
			lengthsq = z.real*z.real+z.imag*z.imag;
			k++;
		    } while (lengthsq < 4.0 && k < 100);
		    if (k==100)						//Store points to draw
			temp[nSize++] = j;
		}
		//Send out points to draw
		MPI_Send(&nSize, 1, MPI_INT, 0, i, MPI_COMM_WORLD);		
		MPI_Send(temp, nSize, MPI_INT, 0, i+X2, MPI_COMM_WORLD);
	    }
	    free(temp);
	    free(tasks);
	}
	// Finalize the MPI environment. No more MPI calls can be made after this
	MPI_Finalize();
  	
	if (world_rank==0)
	{
	    clock_gettime(CLOCK_MONOTONIC, &finish);
	    printf("Time elapsed: %lf.\n", finish.tv_sec-start.tv_sec + (double)(finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	}
	
#ifdef X_ENABLED	
	if (world_rank==0) XFinish();
#endif	
}

void random_shuffle(int *begin, int *end)
{
    int n = end - begin;
    int i, temp, a, b;
    for (i=0; i<n; ++i)
    {
	a = rand()%n;
	b = rand()%n;
	temp = *(begin+a);
	*(begin+a) = *(begin+b);
	*(begin+b) = temp;
    }
}