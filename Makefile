CC=gcc
MPICC=mpicc
CFLAG=-lX11 -lrt -lm -O2 

all: mandelbrot_pthread.c mandelbrot_mpi.c mandelbrot_openmp.c
	make seq
	make pthread
	make mpi
	make openmp

seq: mandelbrot_seq.c
	${CC} mandelbrot_seq.c -o mandelbrot_seq ${CFLAG}

pthread: mandelbrot_pthread.c
	${CC} mandelbrot_pthread.c -o mandelbrot_pthread -lpthread ${CFLAG}
	
mpi: mandelbrot_mpi.c
	${MPICC} mandelbrot_mpi.c -o mandelbrot_mpi ${CFLAG}

openmp: mandelbrot_openmp.c
	${CC} mandelbrot_openmp.c -o mandelbrot_openmp -fopenmp ${CFLAG}

clean:
	rm -f *~
	rm -f mandelbrot_pthread
	rm -f mandelbrot_mpi
	rm -f mandelbrot_openmp
	rm -f mandelbrot_seq
