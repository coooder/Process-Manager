.phony all:
all: PMan

PMan: PMan.c
	gcc -Wall PMan.c -lreadline -o PMan

.PHONY clean:
clean:
	-rm -rf *.o *.exe
