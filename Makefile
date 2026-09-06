CC      = cc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic
TARGET  = scheduler
OBJS    = scheduler.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

scheduler.o: scheduler.c scheduler.h
	$(CC) $(CFLAGS) -c scheduler.c

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: clean
