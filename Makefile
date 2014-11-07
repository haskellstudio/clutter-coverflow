PROGRAM=coverflow
CC=gcc
PACKAGES=clutter-1.0
FLAGS=`pkg-config --cflags $(PACKAGES)` -Wall
LIBS=`pkg-config --libs $(PACKAGES)`

all: $(PROGRAM)

package: all clean
	# todo

clean:
	-rm $(PROGRAM).o

test: all
	-./$(PROGRAM)

$(PROGRAM): $(PROGRAM).o
	$(CC) $^ -o $@ $(LIBS)

$(PROGRAM).o: $(PROGRAM).c
	$(CC) -c $^ -o $@ $(FLAGS)
