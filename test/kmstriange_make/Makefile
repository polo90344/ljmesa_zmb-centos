CC=gcc

SRC = kmstriangle.c
SRC += esTransform.c

CFLAGS = -I ./
CFLAGS += -I /home/orangepi/works/mesa/mesa_install/include/
CFLAGS += -I /usr/include/libdrm

LIB_DIR = -L /home/orangepi/works/mesa/mesa_install/lib/aarch64-linux-gnu/
LIB_NAME= -lEGL -lGLESv2 -lgbm -lm -ldrm


OBJS = $(SRC:%.c=%.o)

$(EXEC): $(OBJS)

all :
	$(CC) $(SRC) $(CFLAGS) $(LIB_DIR) $(LIB_NAME) -o kmstriangle $(LIB_NAME) -g

clean :
	rm -rf *.o
	rm -rf kmstriangle

