CC = g++
CFLAGS = -std=c++11 -pedantic -Wall -Wextra -O2
SRC = src/main.cpp src/ABMOF/abmof.cpp src/ABMOF/abmof_sw.cpp
PROG = abmof_libcaer

INCS = -I./libs/sds_utils/ -I./include/aarch32-linux/include/ -I./include/vivado_include/  

OPENCV = `pkg-config opencv --cflags --libs`
LIBS = -lpthread -lcaer $(OPENCV)

$(PROG):
		$(CC) $(CFLAGS) $(INCS) -o $(PROG) $(SRC) -D_DEFAULT_SOURCE=1 $(LIBS)
