# ex: set ff=dos ts=8 noet:
# $Id$
#

# -fprofile-arcs -ftest-coverage

override CFLAGS = -W -Wall -Wshadow -pedantic -std=gnu99 -g -m32 -I..
override LDFLAGS = -lm -m32 -shared -fPIC -dynamiclib
ALL = int-sqrt.so int-perfect-square.so int-0,1,4,9.so

all: $(ALL)

int-0,1,4,9.so: int-0,1,4,9.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o int-0,1,4,9.so int-0,1,4,9.o

int-perfect-square.so: int-perfect-square.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o int-perfect-square.so int-perfect-square.o

int-sqrt.so: int-sqrt.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o int-sqrt.so int-sqrt.o

clean:
	$(RM) $(ALL) cscope.out *.{gcov,gcda,gcno} *.so *.o

