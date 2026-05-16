CC = gcc
CFLAGS = -Wall -Wextra -std=c11 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0)

SRC = main.c \
      scheduler/priority.c scheduler/srtf.c \
      metrics/metrics.c \
      gui/interface.c gui/gantt.c \
      util/input.c

OBJ = $(SRC:.c=.o)
TARGET = cpu_scheduler_sim

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
