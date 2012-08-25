/* Sequential Mandelbrot program */


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

typedef struct complextype
        {
        float real, imag;
        } Compl;

int X_RESN, Y_RESN;

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

char *bitmap_res;

void main (int argc, char **argv)
{
	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	if ( argc < 3 ) {
	    printf("Usage: %s X_RESN Y_RESN\n", argv[0]);
	    exit(0);
	}
	
    	sscanf(argv[1], "%d", &X_RESN);
	sscanf(argv[2], "%d", &Y_RESN);
    
#ifdef X_ENABLED
	XWindow_Init();
#endif
         
	int i, j, k;
	/* Mandlebrot variables */
	Compl   z, c;
	float   lengthsq, temp;
	
        /* Calculate and draw points */
	bitmap_res = malloc(X_RESN * Y_RESN);
	int ans=0;
	
        for(i=0; i < X_RESN; i++) 
        for(j=0; j < Y_RESN; j++) {
          z.real = z.imag = 0.0;
          c.real = ((float) j - X_RESN/2)/(X_RESN/4);           
          c.imag = ((float) i - Y_RESN/2)/(Y_RESN/4);
          k = 0;

          do  {                                 

            temp = z.real*z.real - z.imag*z.imag + c.real;
            z.imag = 2.0*z.real*z.imag + c.imag;
            z.real = temp;
            lengthsq = z.real*z.real+z.imag*z.imag;
            k++;

          } while (lengthsq < 4.0 && k < 100);
	  
	  bitmap_res[i*Y_RESN+j] = (k==100);
        }
        
#ifdef X_ENABLED	          
        for (i=0; i<X_RESN; ++i)
	    for (j=0; j<Y_RESN; ++j)
		if (bitmap_res[i*Y_RESN+j]) 
		    XDrawPoint (display, win, gc, j, i);
#endif		
         
#ifdef X_ENABLED
        XFinish();
#endif
	
	clock_gettime(CLOCK_MONOTONIC, &finish);
	printf("Time elapsed: %lf.\n", finish.tv_sec-start.tv_sec + (double)(finish.tv_nsec - start.tv_nsec) / 1000000000.0);	
}