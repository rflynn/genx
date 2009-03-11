# ex: set ff=dos ts=8 noet:
# $Id$
#

# -fprofile-arcs -ftest-coverage

override CFLAGS = -W -Wall -Wshadow -pedantic -std=gnu99 -g -m32 -I..
override LDFLAGS = -lm -m32 -shared -fPIC -dynamiclib
ALL = int-sqrt.so

all: $(ALL)

int-sqrt.so: int-sqrt.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o int-sqrt.so int-sqrt.o

clean:
	$(RM) $(ALL) cscope.out *.{gcov,gcda,gcno} *.so *.o

