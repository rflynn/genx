# ex: set ff=dos ts=8 noet:
# $Id$
#

# -fprofile-arcs -ftest-coverage

CFLAGS = -W -Wall -Wshadow -pedantic -std=gnu99 -g -m32 -I..
LDFLAGS = -lm -m32

int-sqrt.so: int-sqrt.o
	$(CC) -o int-sqrt.so -shared -fPIC int-sqrt.o

clean:
	$(RM) $(ALL) $(OBJ) cscope.out *.{gcov,gcda,gcno}

