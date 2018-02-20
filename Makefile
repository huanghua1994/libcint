alignlen = 64

CC  = icc -std=gnu99
CXX = icpc
FC  = ifort

# options for Cori (uses Intel by default)
CC  = cc -std=gnu99
CXX = CC
FC  = ftn

AR  = xiar rcs

INC=-I. -I/global/homes/e/echow/simint-generator/outdir-enuc/build-avx2/install/include

OPTFLAGS = -m64 -qno-offload


#SRC = $(wildcard *.c)
SRC = cint_basisset.c \
      erd_integral.c  \
      oed_integral.c  \
      cint_simint.c
CFLAGS = -O3 -Wall -w2 -qopenmp
CFLAGS += -Wunknown-pragmas -Wunused-variable
CFLAGS += ${OPTFLAGS}
CFLAGS += -D__ALIGNLEN__=${alignlen}

LIBCINT = libcint.a
OBJS := $(addsuffix .o, $(basename $(SRC)))

all: ${LIBCINT} test_cint_simint test_cint_opterd

test_cint_simint: test_cint_simint.o libcint.a
	$(CC) -qopenmp -o test_cint_simint test_cint_simint.o libcint.a -L/global/homes/e/echow/OptErd_Makefile/lib -lerd -loed -L/global/homes/e/echow/simint-generator/outdir-enuc/build-avx2/install/lib64 -lsimint

test_cint_opterd: test_cint_opterd.o libcint.a
	$(CC) -qopenmp -o test_cint_opterd test_cint_opterd.o libcint.a -L/global/homes/e/echow/OptErd_Makefile/lib -lerd -loed

runtest:
	#./test_cint_simint /global/homes/e/echow/gtfock/data/sto-3g.gbs /global/homes/e/echow/gtfock/data/water.xyz
	#./test_cint_simint /global/homes/e/echow/gtfock/data/opt-cc-pvdz/cc-pvdz.gbs /global/homes/e/echow/gtfock/data/1hsg/1hsg_28.xyz
	./test_cint_simint sto-3g-cart.gbs water.xyz
	./test_cint_opterd sto-3g-sph.gbs water.xyz

${LIBCINT}: ${OBJS}
	${AR} $@ $^

%.o : %.c Makefile
	$(CC) ${CFLAGS} ${INC} -c $< -o $@ 

clean:
	rm -f *.o *.s *.d *~ *.a
