CC=g++
INCLUDE = -I ./include/
LIB_DIR = -L /lib/
LIB_NAME=-lljm_gles -lm

all:
	$(CC) -std=c++11 triangle_shader_bin.cpp  $(INCLUDE) $(LIB_DIR) $(LIB_NAME) -o triangle_shader_bin

.PHONY: clean
clean:
	rm -f *.o command.txt vbo.txt

