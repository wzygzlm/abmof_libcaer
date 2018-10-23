CC = g++
CFLAGS = -std=c++11 -g -pedantic -Wall -Wextra -O1
SRC = src/main.cpp src/ABMOF/abmof.cpp src/ABMOF/abmof_sw.cpp
PROG = abmof_libcaer

PROG_READ_DATA = readDataset
SRC_READ_DATA = src/ABMOF/readData.cpp 

INCS = -I./libs/sds_utils/ -I./include/aarch32-linux/include/ -I./include/vivado_include/  

OPENCV = `pkg-config opencv --cflags --libs`
LIBS = -lpthread -lcaer $(OPENCV)

ALL: $(PROG) $(PROG_READ_DATA)

$(PROG): $(SRC)
		$(CC) $(CFLAGS) $(INCS) -o $(PROG) $(SRC) -D_DEFAULT_SOURCE=1 $(LIBS)

$(PROG_READ_DATA): $(SRC_READ_DATA)
		$(CC) $(CFLAGS) $(INCS) -o $(PROG_READ_DATA) $(SRC_READ_DATA) 
