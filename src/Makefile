# ex: set ff=dos ts=8 noet:
# $Id$
#

CFLAGS = -W -Wall -pedantic -std=gnu99 -Wno-unused -O0 -ggdb
LDFLAGS = -lm
BIN = genx
ALL = genx
OBJ = rnd.o x86.o gen.o run.o genx.o

int:
	make "CFLAGS=$(CFLAGS) -DX86_USE_INT" genx

float:
	make "CFLAGS=$(CFLAGS) -DX86_USE_FLOAT" genx

genx: $(OBJ)

rnd.o: rnd.h rnd.c
x86.o: x86.h x86.c
gen.o: gen.h gen.c
run.o: run.h run.c typ.h
genx.o: rnd.o x86.o gen.o

clean:
	$(RM) $(ALL) $(OBJ) cscope.out


