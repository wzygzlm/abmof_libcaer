#ifndef ABMOF_HW_ACCEL
#define ABMOF_HW_ACCEL

#include<stdint.h>
#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"
typedef ap_axiu<64,1,1,1> inputDataElement;
typedef ap_axiu<32,1,1,1> outputDataElement_t;

#define POLARITY_SHIFT 1
#define POLARITY_MASK 0x00000001
#define POLARITY_Y_ADDR_SHIFT 2
#define POLARITY_Y_ADDR_MASK 0x000001FF      //  Reduce mask bit width to reduce LUTs
#define POLARITY_X_ADDR_SHIFT 17
#define POLARITY_X_ADDR_MASK 0x000001FF      //  Reduce mask bit width to reduce LUTs

#define SLICES_NUMBER 4
#define SLICE_WIDTH  256
#define SLICE_HEIGHT 256

#define DVS_WIDTH  240
#define DVS_HEIGHT 180

#define BITS_PER_PIXEL 4
#define COMBINED_PIXELS 32

#define BLOCK_SIZE 11
#define SEARCH_DISTANCE 3
#define AREA_NUMBER 8
#define AREA_SIZE (SLICE_WIDTH/AREA_NUMBER)

#define BLOCK_COL_PIXELS BITS_PER_PIXEL * (BLOCK_SIZE + 2 * SEARCH_DISTANCE)

typedef ap_int<BITS_PER_PIXEL> pix_t;
typedef ap_int<COMBINED_PIXELS * BITS_PER_PIXEL> col_pix_t;
typedef ap_int<COMBINED_PIXELS * BITS_PER_PIXEL * 2> two_cols_pix_t;
typedef ap_uint<2> sliceIdx_t;

typedef ap_int<BLOCK_COL_PIXELS> apIntBlockCol_t;
typedef ap_uint<17> apUint17_t;
typedef ap_uint<15> apUint15_t;
typedef ap_uint<6> apUint6_t;

#define BLOCK_COL_PIXELS BITS_PER_PIXEL * (BLOCK_SIZE + 2 * SEARCH_DISTANCE)
#define PIXS_PER_COL (SLICE_HEIGHT/COMBINED_PIXELS)

void readBlockCols(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdxRef, sliceIdx_t sliceIdxTag, pix_t refCol[BLOCK_SIZE + 2 * SEARCH_DISTANCE]);

void writePix(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdx);
pix_t readPix(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdx);

void topHW(ap_uint<8> x, ap_uint<8> y, sliceIdx_t idx, ap_int<16> *miniSumRet);

void parseEvents(const uint64_t * data, int32_t eventsArraySize, outputDataElement_t *eventSlice);

void parseEventsSW(uint64_t * dataStream, int32_t eventsArraySize, int32_t *eventSlice);

#endif
