CC=gcc
LIBS=-lcmocka -pthread
FLAGS=-std=c17 -g -Wall
ELPATH=..
OBJS=tests.o elite.o macrolist.o

run: tests
	./$?

tests: $(OBJS)
	$(CC) $(FLAGS) -o $@ $(OBJS) $(LIBS)

macrolist.o: $(ELPATH)/macrolist.c
	$(CC) $(FLAGS) -c $?

elite.o: $(ELPATH)/elite.c
	$(CC) $(FLAGS) -c $?

tests.o: tests.c
	$(CC) $(FLAGS) -c $?

.PHONY: clean
clean:
	rm tests $(OBJS)
