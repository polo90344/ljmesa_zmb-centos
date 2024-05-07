CC=gcc

SRC = kmscube.c
SRC += esTransform.c

CFLAGS = -I ./
CFLAGS += -I /usr/include/libdrm

OBJS = $(SRC:%.c=%.o)

$(EXEC): $(OBJS)

all :
	$(CC) $(CFLAGS) $(SRC) -o kmscube -lEGL -lGLESv2 -lgbm -lm -ldrm

clean :
	rm -rf *.o
	rm -rf kmscube

