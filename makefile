LIBS = $(shell ncursesw6-config --libs)
FLAGS = $(shell ncursesw6-config --cflags)

matrix : matrix.o
	cc matrix.o $(LIBS) -o matrix

matrix.o : matrix.c
	cc -c matrix.c $(FLAGS)

clean : 
	rm matrix matrix.o
