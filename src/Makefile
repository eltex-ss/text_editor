objects = main.o

target = text_editor

CFLAGS = -Wall -pedantic -ansi -g

LDLIBS = -lncurses -lform

all : $(target) reloc

$(target) : $(objects)
		$(CC) -o $(target) $(LDLIBS) $^

main.o : %.o : %.c
		$(CC) -c $(CFLAGS) $<

reloc :
		mkdir -p ../build
		mv $(target) ../build

.PHONY : clean

clean :
		-rm -r *.o ../build
