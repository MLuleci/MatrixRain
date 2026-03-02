LIBS = $(shell pkg-config ncursesw --libs)
CFLAGS = $(shell pkg-config ncursesw --cflags)

matrix : matrix.o
	cc matrix.o $(LIBS) -o matrix

matrix.o : matrix.c
	cc -c matrix.c $(CFLAGS)

clean : 
	rm matrix matrix.o
