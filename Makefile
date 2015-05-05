CC = gcc -ggdb -pthread 
CFLAGS = 
OBJM = oss.o
OBJP = userProcess.o
MAIN = oss
PROC = userProcess

.SUFFIXES: .c .o

all: $(MAIN) $(PROC)

$(MAIN): $(OBJM) 
	$(CC) -o $@ $^
$(PROC): $(OBJP)
	$(CC) -o $@ $^
.c.o:
	$(CC) -c -o $@ $<

clean:
	rm -f *.o
	rm -f $(MAIN)
	rm -f $(PROC)

semClean:
	gcc -o semClean semClean.c -pthread
