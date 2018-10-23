#ifndef ABMOF
#define ABMOF

// libcaer
#include <libcaercpp/devices/davis.hpp>

// socket
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>

#include <fstream>
#include <sstream>
#include <vector>

struct SADResult {
	uint16_t dx;
	uint16_t dy;
	bool validFlg;
	uint64_t sadValue;
};

int abmof(int port, int eventThreshold, int socketType);

int init_socket(int port);
void abmof_accel(int16_t x, int16_t y, bool pol, int64_t ts);
void accumulate(int16_t x, int16_t y, bool pol, int64_t ts);
void resetSlices();
void resetCurrentSlice();
void rotateSlices();
SADResult calculateOF(int16_t x, int16_t y, int16_t searchDistance, int16_t blockSize);

using namespace std;

#endif
