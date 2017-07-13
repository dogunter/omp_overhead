.PHONY: all

ifeq ($(PELANL_PRGENV),true)
  CC = cc
  CXX = CC
  FC = ftn
  CFLAGS = -dynamic -std=c99 -O0 -fopenmp
  CXXFLAGS = -dynamic -std=c99 -O0 -fopenmp
  FCFLAGS = -dynamic -O0 -fopenmp
else
  CC = mpicc
  CXX = mpiCC
  FC = mpif90
  CFLAGS = -std=c99 -O0 -fopenmp
  CXXFLAGS = -std=c99 -O0 -fopenmp
  FCFLAGS = -O0 -fopenmp
endif

#INCLUDES=-I$(HWLOCROOT)/include
#LIBDIR=-L$(HWLOCROOT)/lib
#LIBS=-lhwloc -lopen-pal
LIBS=-lhwloc

all: omp_overhead

omp_overhead: omp_overhead.o
	$(CC) $(CFLAGS) -o $@ $@.o $(LIBDIR) $(LIBS)

.c.o:
	${CC} $(INCLUDES) ${CFLAGS} -c $*.c

