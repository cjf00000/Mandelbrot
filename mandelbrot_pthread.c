/* Sequential Mandelbrot program */


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

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
int 		tasks[MAX_THREADS];
int		taskRemaining;
pthread_mutex_t	mutexJob, mutexX;

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

void* calcFrac(void *x)
{
    int i, j, k;
    /* Mandlebrot variables */
    Compl   z, c;
    float   lengthsq, temp;
    i = *((int*)x);
    
    while (1)
    {	
	for(j=0; j < Y_RESN; j++) {	    
	    z.real = z.imag = 0.0;
	    c.real = ((float) j - Y_RESN/2)/(Y_RESN/4);               /* scale factors for 800 x 800 window */
	    c.imag = ((float) i - X_RESN/2)/(X_RESN/4);
	    k = 0;

	    do  {                                             /* iterate for pixel color */
	    temp = z.real*z.real - z.imag*z.imag + c.real;
	    z.imag = 2.0*z.real*z.imag + c.imag;
	    z.real = temp;
	    lengthsq = z.real*z.real+z.imag*z.imag;
	    k++;
	    } while (lengthsq < 4.0 && k < 100);

#ifdef X_ENABLED
	    //XWindow Draw
	    pthread_mutex_lock(&mutexX);
	    if (k == 100) XDrawPoint (display, win, gc, j, i);
	    pthread_mutex_unlock(&mutexX);
#endif
	}
	
	// Find another job    
	pthread_mutex_lock(&mutexJob);
	if (taskRemaining < 0)
	{
	    pthread_mutex_unlock(&mutexJob);
	    pthread_exit(NULL);
	}
	else
	{
	    i = taskRemaining--;
	}	
	pthread_mutex_unlock(&mutexJob);
    }
}

void XFinish()
{
    XFlush (display);
    sleep (30);	
}

void main (int argc, char **argv)
{
	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	if ( argc < 4 ) {
	    printf("Usage: %s X_RESN Y_RESN nThreads\n", argv[0]);
	    exit(0);
	}
	sscanf(argv[1], "%d", &X_RESN);
	sscanf(argv[2], "%d", &Y_RESN);
	sscanf(argv[3], "%d", &nThreads);
	nThreads = min(nThreads, X_RESN);

#ifdef X_ENABLED	
	XWindow_Init();	
#endif
	
	pthread_mutex_init(&mutexJob, NULL);	
	pthread_mutex_init(&mutexX, NULL);	
	
	//Create Threads
	pthread_t *threads = malloc( sizeof(pthread_t) * nThreads );
	int delta = X_RESN / nThreads;
	int i;
	for ( i=0; i<nThreads; i++ )
	{
	    tasks[i] = X_RESN - i - 1;
	    pthread_create( &threads[i], NULL, calcFrac, (void*)&tasks[i] );
	}
	taskRemaining = X_RESN - nThreads;	
	
	for ( i=0; i<nThreads; ++i )
	    pthread_join(threads[i], NULL);

#ifdef X_ENABLED	
	XFinish();
#endif
	
	clock_gettime(CLOCK_MONOTONIC, &finish);
	printf("Time elapsed: %lf.\n", finish.tv_sec-start.tv_sec + (double)(finish.tv_nsec - start.tv_nsec) / 1000000000.0);
	pthread_exit(NULL);
}
