# ex: set ff=dos ts=8 noet:
# $Id$
#

# -fprofile-arcs -ftest-coverage

CFLAGS = -W -Wall -Wshadow -pedantic -std=gnu99 -ggdb -m32
LDFLAGS = -lm -m32 -ldl -ggdb
BIN = genx
ALL = genx
OBJ = rnd.o x86.o gen.o run.o genx.o

debug:
	$(MAKE) "CFLAGS=$(CFLAGS) -O0" int

fast:
	$(MAKE) "CFLAGS=$(CFLAGS) -Os" int

int:
	$(MAKE) "CFLAGS=$(CFLAGS) -DX86_USE_INT" genx
	$(MAKE) -C problems

float:
	$(MAKE) "CFLAGS=$(CFLAGS) -DX86_USE_FLOAT" genx
	$(MAKE) -C problems

genx: $(OBJ)

clean:
	$(MAKE) -C problems clean
	$(RM) $(ALL) $(OBJ) cscope.out *.{gcov,gcda,gcno}

