objects = main.o list.o

target = text_editor

CFLAGS = -Wall -pedantic -ansi -g -std=c99

LDLIBS = -lncurses

all : $(target) reloc

$(target) : $(objects)
		$(CC) -o $(target) $(LDLIBS) $^

$(objects) : list.h
$(objects) : %.o : %.c
		$(CC) -c $(CFLAGS) $<

reloc :
		mkdir -p ../build
		mv $(target) ../build

.PHONY : clean

clean :
		-rm -r *.o ../build
