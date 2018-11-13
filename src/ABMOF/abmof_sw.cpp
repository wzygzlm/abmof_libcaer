#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
using namespace std;
#include "abmof.h"
#include "abmof_hw_accel.h"
#include "time.h"

#define TEST_TIMES 20

static col_pix_t slicesSW[SLICES_NUMBER][SLICE_WIDTH][SLICE_HEIGHT/COMBINED_PIXELS];
static col_pix_t slicesScale1SW[SLICES_NUMBER][SLICE_WIDTH/2][SLICE_HEIGHT/COMBINED_PIXELS/2];
static col_pix_t slicesScale2SW[SLICES_NUMBER][SLICE_WIDTH/4][SLICE_HEIGHT/COMBINED_PIXELS/4];
static sliceIdx_t glPLActiveSliceIdxSW = 0;

void resetPixSW(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdx)
{
	slicesSW[sliceIdx][x][y/COMBINED_PIXELS] = 0;
	slicesScale1SW[sliceIdx][x/2][y/COMBINED_PIXELS/2] = 0;
	slicesScale2SW[sliceIdx][x/4][y/COMBINED_PIXELS/4] = 0;
}

void writePixSW(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdx)
{
    // write scale 0
	int8_t yNewIdx = y%COMBINED_PIXELS;
//	cout << "Data before write : " << slicesSW[sliceIdx][x][y/COMBINED_PIXELS].range(4 * yNewIdx + 3, 4 * yNewIdx) << endl;
	pix_t tmp = slicesSW[sliceIdx][x][y/COMBINED_PIXELS].range(4 * yNewIdx + 3, 4 * yNewIdx);
    if (tmp >= (ap_uint< BITS_PER_PIXEL - 1 >(0xff))) tmp = (ap_uint< BITS_PER_PIXEL - 1 >(0xff));
    else tmp += 1;
	slicesSW[sliceIdx][x][y/COMBINED_PIXELS].range(4 * yNewIdx + 3, 4 * yNewIdx) = tmp;
//	cout << "Data after write : " << slicesSW[sliceIdx][x][y/COMBINED_PIXELS].range(4 * yNewIdx + 3, 4 * yNewIdx) << endl;

    // write scale 1
    ap_uint<8> xScale1 = x/2;
    ap_uint<8> yScale1 = y/2;
	int8_t yNewIdxScale1 = yScale1%COMBINED_PIXELS;
//	cout << "Data before write : " << slicesScale1SW[sliceIdx][xScale1][yScale1/COMBINED_PIXELS].range(4 * yNewIdxScale1 + 3, 4 * yNewIdxScale1) << endl;
	pix_t tmpScale1 = slicesScale1SW[sliceIdx][xScale1][yScale1/COMBINED_PIXELS].range(4 * yNewIdxScale1 + 3, 4 * yNewIdxScale1);
    if (tmpScale1 >= (ap_uint< BITS_PER_PIXEL - 1 >(0xff))) tmpScale1 = (ap_uint< BITS_PER_PIXEL - 1 >(0xff));
    else tmpScale1 += 1;
	slicesScale1SW[sliceIdx][xScale1][yScale1/COMBINED_PIXELS].range(4 * yNewIdxScale1 + 3, 4 * yNewIdxScale1) = tmpScale1;
//	cout << "Data after write : " << slicesScale1SW[sliceIdx][xScale1][yScale1/COMBINED_PIXELS].range(4 * yNewIdxScale1 + 3, 4 * yNewIdxScale1) << endl;

    // write scale 2
    ap_uint<8> xScale2 = x/4;
    ap_uint<8> yScale2 = y/4;
	int8_t yNewIdxScale2 = yScale2%COMBINED_PIXELS;
//	cout << "Data before write : " << slicesScale2SW[sliceIdx][xScale2][yScale2/COMBINED_PIXELS].range(4 * yNewIdxScale2 + 3, 4 * yNewIdxScale2) << endl;
	pix_t tmpScale2 = slicesScale2SW[sliceIdx][xScale2][yScale2/COMBINED_PIXELS].range(4 * yNewIdxScale2 + 3, 4 * yNewIdxScale2);
    if (tmpScale2 >= (ap_uint< BITS_PER_PIXEL - 1 >(0xff))) tmpScale2 = (ap_uint< BITS_PER_PIXEL - 1 >(0xff));
    else tmpScale2 += 1;
	slicesScale2SW[sliceIdx][xScale2][yScale2/COMBINED_PIXELS].range(4 * yNewIdxScale2 + 3, 4 * yNewIdxScale2) = tmpScale2;
//	cout << "Data after write : " << slicesScale2SW[sliceIdx][xScale2][yScale2/COMBINED_PIXELS].range(4 * yNewIdxScale2 + 3, 4 * yNewIdxScale2) << endl;
}

void readBlockColsSWScale0(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdxRef, sliceIdx_t sliceIdxTag,
		pix_t refCol[BLOCK_SIZE + 2 * SEARCH_DISTANCE], pix_t tagCol[BLOCK_SIZE + 2 * SEARCH_DISTANCE])
{

	two_cols_pix_t refColData;
    two_cols_pix_t tagColData;
    ap_uint<3> neighboryOffset;
    if ( y%COMBINED_PIXELS < BLOCK_SIZE/2 + SEARCH_DISTANCE )
    {
        neighboryOffset = y/COMBINED_PIXELS - 1;
        // concatenate two columns together
        refColData = (slicesSW[sliceIdxRef][x][y/COMBINED_PIXELS], slicesSW[sliceIdxRef][x][neighboryOffset]); 
        //	cout << "refColData: " << refColData << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColData = (slicesSW[(sliceIdx_t)(sliceIdxTag + 0)][x][y/COMBINED_PIXELS], slicesSW[(sliceIdx_t)(sliceIdxTag + 0)][x][neighboryOffset]);
    }
    else if ( y%COMBINED_PIXELS >  COMBINED_PIXELS - BLOCK_SIZE/2 - SEARCH_DISTANCE - 1 )
    {
        neighboryOffset = y/COMBINED_PIXELS + 1;
        // concatenate two columns together
        refColData = (slicesSW[sliceIdxRef][x][y/COMBINED_PIXELS], slicesSW[sliceIdxRef][x][neighboryOffset]); 
        //	cout << "refColData: " << refColData << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColData = (slicesSW[(sliceIdx_t)(sliceIdxTag + 0)][x][y/COMBINED_PIXELS], slicesSW[(sliceIdx_t)(sliceIdxTag + 0)][x][neighboryOffset]);
    }
    else
    {
        neighboryOffset = y/COMBINED_PIXELS + 0;
        refColData = (slicesSW[sliceIdxRef][x][y/COMBINED_PIXELS], slicesSW[sliceIdxRef][x][neighboryOffset]); 
        //	cout << "refColData: " << refColData << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColData = (slicesSW[(sliceIdx_t)(sliceIdxTag + 0)][x][y/COMBINED_PIXELS], slicesSW[(sliceIdx_t)(sliceIdxTag + 0)][x][neighboryOffset]);
    }


	// find the bottom pixel of the column that centered on y.
	ap_uint<6> yColOffsetIdx = y%COMBINED_PIXELS - BLOCK_SIZE/2 - SEARCH_DISTANCE + COMBINED_PIXELS;

	readRefLoop: for(ap_uint<8> i = 0; i < BLOCK_SIZE + 2 * SEARCH_DISTANCE; i++)
	{
		refCol[i] = refColData.range(yColOffsetIdx * 4 + 3, yColOffsetIdx * 4);
		tagCol[i] = tagColData.range(yColOffsetIdx * 4 + 3, yColOffsetIdx * 4);
		yColOffsetIdx++;
	}
}

void readBlockColsSWScale1(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdxRef, sliceIdx_t sliceIdxTag,
		pix_t refColScale1[BLOCK_SIZE + 2 * SEARCH_DISTANCE], pix_t tagColScale1[BLOCK_SIZE + 2 * SEARCH_DISTANCE])
{
	two_cols_pix_t refColDataScale1;
    two_cols_pix_t tagColDataScale1;
    ap_uint<3> neighboryOffsetScale1;
    ap_uint<8> xScale1 = x;
    ap_uint<8> yScale1 = y;
    if ( yScale1%COMBINED_PIXELS < BLOCK_SIZE/2 + SEARCH_DISTANCE )
    {
        neighboryOffsetScale1 = yScale1/COMBINED_PIXELS - 1;
        // concatenate two columns together
        refColDataScale1 = (slicesScale1SW[sliceIdxRef][xScale1][yScale1/COMBINED_PIXELS], slicesScale1SW[sliceIdxRef][xScale1][neighboryOffsetScale1]); 
        //	cout << "refColDataScale1: " << refColDataScale1 << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColDataScale1 = (slicesScale1SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale1][yScale1/COMBINED_PIXELS], slicesScale1SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale1][neighboryOffsetScale1]);
    }
    else if ( yScale1%COMBINED_PIXELS >  COMBINED_PIXELS - BLOCK_SIZE/2 - SEARCH_DISTANCE - 1 )
    {
        neighboryOffsetScale1 = yScale1/COMBINED_PIXELS + 1;
        // concatenate two columns together
        refColDataScale1 = (slicesScale1SW[sliceIdxRef][xScale1][yScale1/COMBINED_PIXELS], slicesScale1SW[sliceIdxRef][xScale1][neighboryOffsetScale1]); 
        //	cout << "refColDataScale1: " << refColDataScale1 << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColDataScale1 = (slicesScale1SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale1][yScale1/COMBINED_PIXELS], slicesScale1SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale1][neighboryOffsetScale1]);
    }
    else
    {
        neighboryOffsetScale1 = yScale1/COMBINED_PIXELS + 0;
        refColDataScale1 = (slicesScale1SW[sliceIdxRef][xScale1][yScale1/COMBINED_PIXELS], slicesScale1SW[sliceIdxRef][xScale1][neighboryOffsetScale1]); 
        //	cout << "refColDataScale1: " << refColDataScale1 << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColDataScale1 = (slicesScale1SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale1][yScale1/COMBINED_PIXELS], slicesScale1SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale1][neighboryOffsetScale1]);
    }

	// find the bottom pixel of the column that centered on y.
	ap_uint<6> yColOffsetIdxScale1 = yScale1%COMBINED_PIXELS - BLOCK_SIZE/2 - SEARCH_DISTANCE + COMBINED_PIXELS;

	readRefLoop: for(ap_uint<8> i = 0; i < BLOCK_SIZE + 2 * SEARCH_DISTANCE; i++)
	{
		refColScale1[i] = refColDataScale1.range(yColOffsetIdxScale1 * 4 + 3, yColOffsetIdxScale1 * 4);
		tagColScale1[i] = tagColDataScale1.range(yColOffsetIdxScale1 * 4 + 3, yColOffsetIdxScale1 * 4);
		yColOffsetIdxScale1++;
	}
}

void readBlockColsSWScale2(ap_uint<8> x, ap_uint<8> y, sliceIdx_t sliceIdxRef, sliceIdx_t sliceIdxTag,
		pix_t refColScale2[BLOCK_SIZE + 2 * SEARCH_DISTANCE], pix_t tagColScale2[BLOCK_SIZE + 2 * SEARCH_DISTANCE])
{
	two_cols_pix_t refColDataScale2;
    two_cols_pix_t tagColDataScale2;
    ap_uint<3> neighboryOffsetScale2;
    ap_uint<8> xScale2 = x;
    ap_uint<8> yScale2 = y;
    if ( yScale2%COMBINED_PIXELS < BLOCK_SIZE/2 + SEARCH_DISTANCE )
    {
        neighboryOffsetScale2 = yScale2/COMBINED_PIXELS - 1;
        // concatenate two columns together
        refColDataScale2 = (slicesScale2SW[sliceIdxRef][xScale2][yScale2/COMBINED_PIXELS], slicesScale2SW[sliceIdxRef][xScale2][neighboryOffsetScale2]); 
        //	cout << "refColDataScale2: " << refColDataScale2 << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColDataScale2 = (slicesScale2SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale2][yScale2/COMBINED_PIXELS], slicesScale2SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale2][neighboryOffsetScale2]);
    }
    else if ( yScale2%COMBINED_PIXELS >  COMBINED_PIXELS - BLOCK_SIZE/2 - SEARCH_DISTANCE - 1 )
    {
        neighboryOffsetScale2 = yScale2/COMBINED_PIXELS + 1;
        // concatenate two columns together
        refColDataScale2 = (slicesScale2SW[sliceIdxRef][xScale2][yScale2/COMBINED_PIXELS], slicesScale2SW[sliceIdxRef][xScale2][neighboryOffsetScale2]); 
        //	cout << "refColDataScale2: " << refColDataScale2 << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColDataScale2 = (slicesScale2SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale2][yScale2/COMBINED_PIXELS], slicesScale2SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale2][neighboryOffsetScale2]);
    }
    else
    {
        neighboryOffsetScale2 = yScale2/COMBINED_PIXELS + 0;
        refColDataScale2 = (slicesScale2SW[sliceIdxRef][xScale2][yScale2/COMBINED_PIXELS], slicesScale2SW[sliceIdxRef][xScale2][neighboryOffsetScale2]); 
        //	cout << "refColDataScale2: " << refColDataScale2 << endl; 
        // concatenate two columns together
        // Use explicit cast here, otherwise it will generate a lot of select operations which consumes more LUTs than MUXs.
        tagColDataScale2 = (slicesScale2SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale2][yScale2/COMBINED_PIXELS], slicesScale2SW[(sliceIdx_t)(sliceIdxTag + 0)][xScale2][neighboryOffsetScale2]);
    }

	// find the bottom pixel of the column that centered on y.
	ap_uint<6> yColOffsetIdxScale2 = yScale2%COMBINED_PIXELS - BLOCK_SIZE/2 - SEARCH_DISTANCE + COMBINED_PIXELS;

	readRefLoop: for(ap_uint<8> i = 0; i < BLOCK_SIZE + 2 * SEARCH_DISTANCE; i++)
	{
		refColScale2[i] = refColDataScale2.range(yColOffsetIdxScale2 * 4 + 3, yColOffsetIdxScale2 * 4);
		tagColScale2[i] = tagColDataScale2.range(yColOffsetIdxScale2 * 4 + 3, yColOffsetIdxScale2 * 4);
		yColOffsetIdxScale2++;
	}
}

void colSADSumSW(pix_t in1[BLOCK_SIZE + 2 * SEARCH_DISTANCE],
		pix_t in2[BLOCK_SIZE + 2 * SEARCH_DISTANCE],
		int16_t out[2 * SEARCH_DISTANCE + 1])
{
//	cout << "in1 is: " << endl;
//	for (int m = 0; m < BLOCK_SIZE + 2 * SEARCH_DISTANCE; m++)
//	{
//		cout << in1[m] << " ";
//	}
//	cout << endl;
//
//	cout << "in2 is: " << endl;
//	for (int m = 0; m < BLOCK_SIZE + 2 * SEARCH_DISTANCE; m++)
//	{
//		cout << in2[m] << " ";
//	}
//	cout << endl;

	for(int i = 0; i <= 2 * SEARCH_DISTANCE; i++)
	{
		int16_t tmpOut = 0;
		for(int j = 0; j < BLOCK_SIZE; j++)
		{
			tmpOut += abs(in1[j + SEARCH_DISTANCE] - in2[i+j]);  // in1 should get the col data centered on current event.
		}
//		cout << "tmpOut is " << tmpOut << endl;
		out[i] = tmpOut;
	}
}

// Set the initial value as the max integer, cannot be 0x7fff, DON'T KNOW WHY.
static ap_int<16> miniRetVal = 0x7fff;
static ap_uint<6> minOFRet = ap_uint<6>(0xff);

static ap_int<16> miniSumTmp[2*SEARCH_DISTANCE + 1] = {0, 0, 0, 0, 0, 0, 0};
static ap_int<16> localSumReg[BLOCK_SIZE][2*SEARCH_DISTANCE + 1];

static int16_t testTmpSum;
void miniSADSumSW(pix_t in1[BLOCK_SIZE + 2 * SEARCH_DISTANCE],
		pix_t in2[BLOCK_SIZE + 2 * SEARCH_DISTANCE],
		int16_t shiftCnt,
		ap_int<16> *miniSumRet,
		ap_uint<6> *OFRet)
{
	int16_t out[2*SEARCH_DISTANCE + 1];

	colSADSumSW(in1, in2, out);

	ap_uint<1> cond1 = (shiftCnt >= BLOCK_SIZE - 1) ? 1 : 0;
	for(int8_t i = 0; i < BLOCK_SIZE - 1; i++)
	{
		shiftInnerLoop: for(int8_t j = 0; j <= 2*SEARCH_DISTANCE; j++)
		{
			localSumReg[i][j] = localSumReg[i + 1][j];
		}
	}

	for(int8_t j = 0; j <= 2*SEARCH_DISTANCE; j++)
	{
		localSumReg[BLOCK_SIZE - 1][j] = out[j];
	}


	ap_uint<3> OFRet_x = minOFRet.range(2, 0);
	ap_uint<3> OFRet_y = minOFRet.range(5, 3);

	for(int8_t i = 0; i <= 2*SEARCH_DISTANCE; i++)
	{
		miniSumTmp[i] = 0;
		for(int8_t j = 0; j <= BLOCK_SIZE - 1; j++)
		{
			miniSumTmp[i] += localSumReg[j][i];
		}
	}

//	cout << "miniSumTmp is: " << endl;
//	for (int m = 0; m <= 2 * SEARCH_DISTANCE; m++)
//	{
//		cout << miniSumTmp[m] << " ";
//	}
//	cout << endl;

	// Find the minimal of current column.
	ap_int<16> miniRetValTmpIter = ap_int<16>(*min_element(miniSumTmp, miniSumTmp + 2*SEARCH_DISTANCE + 1));
	int miniIdx = distance(miniSumTmp, min_element(miniSumTmp, miniSumTmp + 2*SEARCH_DISTANCE + 1));

	// Compare with current global minimum value.
	if (miniRetValTmpIter < miniRetVal)
	{
		if((shiftCnt >= BLOCK_SIZE - 1))
		{
			miniRetVal = miniRetValTmpIter;     // Update the global value
			OFRet_x = ap_uint<3>(shiftCnt - BLOCK_SIZE + 1);   // Record the shift value and store it in OFRet_x
			OFRet_y = ap_uint<3>(miniIdx);
			minOFRet = OFRet_y.concat(OFRet_x);     // Update the OF value.
//			cout << "OF and global minimum updated at index shiftCnt: " << shiftCnt << endl;
		}
	}

//	cout << "OF_x is: " << OFRet_x << "\t OF_y is: " << OFRet_y << endl;

	*miniSumRet = miniRetVal;
	*OFRet = minOFRet;

//	std::cout << "miniSumRetSW is: " << *miniSumRet << "\t OFRetSW is: " << std::hex << *OFRet << std::endl;
//	std::cout << std::dec;    // Restore dec mode
}

void blockSADSW(pix_t blockIn1[BLOCK_SIZE][BLOCK_SIZE], pix_t blockIn2[BLOCK_SIZE][BLOCK_SIZE], uint16_t *sumRet)
{
    uint16_t tmpSum = 0;
    uint16_t validPixRefBlockCnt = 0, validPixTagBlockCnt = 0, nonZeroMatchCnt = 0;
    for(uint8_t i = 0; i < BLOCK_SIZE; i++)
    {
        for(uint8_t j = 0; j < BLOCK_SIZE; j++)
        {
            tmpSum += abs(blockIn1[i][j] - blockIn2[i][j]);

            if (blockIn1[i][j] != 0)
            {
                validPixRefBlockCnt++;
            }
            if (blockIn2[i][j] != 0)
            {
                validPixTagBlockCnt++;
            }
            if (blockIn1[i][j] != 0 && blockIn2[i][j] != 0)
            {
                nonZeroMatchCnt++;
            }
        }
    }

    // Remove outliers
    int minValidPixNum = 0.02 * (BLOCK_SIZE * BLOCK_SIZE); 
    if (validPixRefBlockCnt < minValidPixNum || validPixTagBlockCnt < minValidPixNum || nonZeroMatchCnt < minValidPixNum)
    {
        tmpSum = 0x7fff;
    } 
    *sumRet = tmpSum;
}

void miniBlockSADSW(pix_t refBlock[BLOCK_SIZE][BLOCK_SIZE],
        pix_t tagBlock[BLOCK_SIZE + 2 * SEARCH_DISTANCE][BLOCK_SIZE + 2 * SEARCH_DISTANCE], bool printBlocksEnable,
        ap_int<16> *miniRet, ap_uint<6> *OFRet)
{
    uint16_t tmpSum = 0x7fff;
    ap_uint<3> tmpOF_x = ap_uint<3>(7);
    ap_uint<3> tmpOF_y = ap_uint<3>(7);

    if(printBlocksEnable == true)
    {
        cout << "Reference block is: " << endl;
        for(uint8_t blockX = 0; blockX < BLOCK_SIZE; blockX++)
        {
            for(uint8_t blockY = 0; blockY < BLOCK_SIZE; blockY++)
            {
                cout << refBlock[blockX][blockY] << "\t";
            }
            cout << endl;
        }
        cout << endl;

        cout << "target block is: " << endl;
        for(uint8_t blockX = 0; blockX < BLOCK_SIZE + 2 * SEARCH_DISTANCE; blockX++)
        {
            for(uint8_t blockY = 0; blockY < BLOCK_SIZE + 2 * SEARCH_DISTANCE; blockY++)
            {
                cout << tagBlock[blockX][blockY] << "\t";
            }
            cout << endl;
        }
        cout << endl;
    }

    for(uint8_t xOffset = 0; xOffset < 2 * SEARCH_DISTANCE + 1; xOffset++)
    {
        for(uint8_t yOffset = 0; yOffset < 2 * SEARCH_DISTANCE + 1; yOffset++)
        {
            pix_t tagBlockIn[BLOCK_SIZE][BLOCK_SIZE];
            uint16_t tmpBlockSum;
            for(uint8_t i = 0; i < BLOCK_SIZE; i++)
            {
                for(uint8_t j = 0; j < BLOCK_SIZE; j++)
                {
                    tagBlockIn[i][j] = tagBlock[i + xOffset][j + yOffset];
                }
            }

            blockSADSW(refBlock, tagBlockIn, &tmpBlockSum);

            if(tmpBlockSum < tmpSum)
            {
                tmpSum = tmpBlockSum;
                tmpOF_x = ap_uint<3>(xOffset);
                tmpOF_y = ap_uint<3>(yOffset);
            }
        }
    }

    if(tmpSum == 0x7fff)
    {
        tmpOF_x = 7;
        tmpOF_y = 7;
    }
    
    *miniRet = tmpSum;
    *OFRet = tmpOF_y.concat(tmpOF_x);
//	std::cout << "miniSumRetSW is: " << *miniRet << "\t OFRetSW is: " << std::hex << *OFRet << std::endl;
//	std::cout << std::dec;    // Restore dec mode
}



void testMiniSADSumWrapperSW(apIntBlockCol_t *input1, apIntBlockCol_t *input2, int16_t eventCnt, apUint15_t *miniSum, apUint6_t *OF)
{
	pix_t ref[BLOCK_SIZE + 2 * SEARCH_DISTANCE], tag[BLOCK_SIZE + 2 * SEARCH_DISTANCE];

	apIntBlockCol_t inData1, inData2;
	ap_int<16> miniSumSWRet;
	ap_uint<6> OFRetSWRet;

	for(int32_t i = 0; i < eventCnt; i++)
	{
		// Initialize the localSumReg
		for(int idx1 = 0; idx1 < BLOCK_SIZE; idx1++)
		{
			for(int idx2 = 0; idx2 < BLOCK_SIZE; idx2++)
			{
				localSumReg[idx1][idx2] = 0;
			}
		}
		miniRetVal = 0x7fff;
		minOFRet = ap_uint<6>(0xff);

		for(int shiftOffset = 0; shiftOffset < BLOCK_SIZE + 2 * SEARCH_DISTANCE; shiftOffset++)
		{
			cout << "current iteration index is: " << i * (BLOCK_SIZE + 2 * SEARCH_DISTANCE) + shiftOffset << endl;
			inData1 = *input1++;
			inData2 = *input2++;

			for(int j = 0; j < BLOCK_SIZE + 2 * SEARCH_DISTANCE; j++)
			{
				ref[j] = pix_t(inData1.range(4*j + 3, 4*j));
				tag[j] = pix_t(inData2.range(4*j + 3, 4*j));
			}

			miniSADSumSW(ref, tag, shiftOffset, &miniSumSWRet, &OFRetSWRet);
		}

		std::cout << "miniSumRetSW is: " << apUint15_t(miniSumSWRet) << "\t OFRetSW is: " << std::hex << OFRetSWRet << std::endl;
		cout << dec;

		miniSum[i] = apUint15_t(miniSumSWRet);
		OF[i] = OFRetSWRet;
	}
}

void testSingleRwslicesSW(ap_uint<8> x, ap_uint<8> y, sliceIdx_t idx, pix_t refCol[BLOCK_SIZE + 2 * SEARCH_DISTANCE], pix_t tagCol[BLOCK_SIZE + 2 * SEARCH_DISTANCE])
{
	writePixSW(x, y, idx);
	readBlockColsSWScale0(x, y, idx + 1, idx + 2, refCol, tagCol);
	resetPixSW(x, y, idx + 3);
}

void testRwslicesSW(uint64_t * data, sliceIdx_t idx, int16_t eventCnt, apIntBlockCol_t *refData, apIntBlockCol_t *tagData)
{
	// Check the accumulation slice is clear or not
	for(int32_t xAddr = 0; xAddr < SLICE_WIDTH; xAddr++)
	{
		for(int32_t yAddr = 0; yAddr < SLICE_HEIGHT; yAddr = yAddr + COMBINED_PIXELS)
		{
			if (slicesSW[idx][xAddr][yAddr/COMBINED_PIXELS] != 0)
			{
				cout << "Ha! I caught you, the pixel which is not clear!" << endl;
				cout << "x is: " << xAddr << "\t y is: " << yAddr << "\t idx is: " << idx << endl;
			}
		}
	}

	for(int32_t i = 0; i < eventCnt; i++)
	{
		uint64_t tmp = *data++;
		ap_uint<8> xWr, yWr;
		xWr = ((tmp) >> POLARITY_X_ADDR_SHIFT) & POLARITY_X_ADDR_MASK;
		yWr = ((tmp) >> POLARITY_Y_ADDR_SHIFT) & POLARITY_Y_ADDR_MASK;
		bool pol  = ((tmp) >> POLARITY_SHIFT) & POLARITY_MASK;
		int64_t ts = tmp >> 32;

		writePixSW(xWr, yWr, idx);

		resetPixSW(i/(PIXS_PER_COL), (i % (PIXS_PER_COL)) * COMBINED_PIXELS, (sliceIdx_t)(idx + 3));
//		cout << "tmp is: " << hex << tmp << endl;
//		cout << "x is: " << xWr << "\t y is: " << yWr << "\t idx is: " << idx << endl;

		for(int8_t xOffSet = 0; xOffSet < BLOCK_SIZE + 2 * SEARCH_DISTANCE; xOffSet++)
		{
            pix_t out1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

            pix_t out1Scale1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2Scale1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

            pix_t out1Scale2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2Scale2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

//				resetPix(xRd + xOffSet, yRd , (sliceIdx_t)(idx + 3));

//			resetPix(xRd + xOffSet, 1 , (sliceIdx_t)(idx + 3));

			readBlockColsSWScale0(xWr + xOffSet, yWr , idx + 1, idx + 2, out1, out2);


			apIntBlockCol_t refBlockCol;
			apIntBlockCol_t tagBlockCol;

			for (int8_t l = 0; l < BLOCK_SIZE + 2 * SEARCH_DISTANCE; l++)
			{
				refBlockCol.range(BITS_PER_PIXEL * l + BITS_PER_PIXEL - 1, BITS_PER_PIXEL * l) = out1[l];
				tagBlockCol.range(BITS_PER_PIXEL * l + BITS_PER_PIXEL - 1, BITS_PER_PIXEL * l) = out2[l];
			}

			*refData++ = refBlockCol;
			*tagData++ = tagBlockCol;
		}
	}


	for (int16_t resetCnt = 0; resetCnt < 2048; resetCnt = resetCnt + 2)
	{
		resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL) * COMBINED_PIXELS, (sliceIdx_t)(idx + 3));
		resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL + 1) * COMBINED_PIXELS, (sliceIdx_t)(idx + 3));
	}
}

void testTempSW(uint64_t * data, sliceIdx_t idx, int16_t eventCnt, int32_t *eventSlice)
{
	// Check the accumulation slice is clear or not
	for(int32_t xAddr = 0; xAddr < SLICE_WIDTH; xAddr++)
	{
		for(int32_t yAddr = 0; yAddr < SLICE_HEIGHT; yAddr = yAddr + COMBINED_PIXELS)
		{
			if (slicesSW[idx][xAddr][yAddr/COMBINED_PIXELS] != 0)
			{
				for(int r = 0; r < 1000; r++)
				{
					cout << "Ha! I caught you, the pixel which is not clear!" << endl;
					cout << "x is: " << xAddr << "\t y is: " << yAddr << "\t idx is: " << idx << endl;
				}
			}
		}
	}

	ap_int<16> miniSumSWRet;
	ap_uint<6> OFRetSWRet;

	for(int32_t i = 0; i < eventCnt; i++)
	{
		uint64_t tmp = *data++;
		ap_uint<8> xWr, yWr;
		xWr = ((tmp) >> POLARITY_X_ADDR_SHIFT) & POLARITY_X_ADDR_MASK;
		yWr = ((tmp) >> POLARITY_Y_ADDR_SHIFT) & POLARITY_Y_ADDR_MASK;
		bool pol  = ((tmp) >> POLARITY_SHIFT) & POLARITY_MASK;
		int64_t ts = tmp >> 32;

		writePixSW(xWr, yWr, idx);

		resetPixSW(i/(PIXS_PER_COL), (i % (PIXS_PER_COL)) * COMBINED_PIXELS, (sliceIdx_t)(idx + 3));
//		cout << "tmp is: " << hex << tmp << endl;
//		cout << "x is: " << xWr << "\t y is: " << yWr << "\t idx is: " << idx << endl;

		// Initialize the localSumReg
		for(int idx1 = 0; idx1 < BLOCK_SIZE; idx1++)
		{
			for(int idx2 = 0; idx2 < BLOCK_SIZE; idx2++)
			{
				localSumReg[idx1][idx2] = 0;
			}
		}
		miniRetVal = 0x7fff;
		minOFRet = ap_uint<6>(0xff);

		for(int8_t xOffSet = 0; xOffSet < BLOCK_SIZE + 2 * SEARCH_DISTANCE; xOffSet++)
		{
            pix_t out1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

            pix_t out1Scale1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2Scale1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

            pix_t out1Scale2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2Scale2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

//				resetPix(xRd + xOffSet, yRd , (sliceIdx_t)(idx + 3));

//			resetPix(xRd + xOffSet, 1 , (sliceIdx_t)(idx + 3));

			readBlockColsSWScale0(xWr + xOffSet, yWr , idx + 1, idx + 2, out1, out2);


			apIntBlockCol_t refBlockCol;
			apIntBlockCol_t tagBlockCol;

			for (int8_t l = 0; l < BLOCK_SIZE + 2 * SEARCH_DISTANCE; l++)
			{
				refBlockCol.range(BITS_PER_PIXEL * l + BITS_PER_PIXEL - 1, BITS_PER_PIXEL * l) = out1[l];
				tagBlockCol.range(BITS_PER_PIXEL * l + BITS_PER_PIXEL - 1, BITS_PER_PIXEL * l) = out2[l];
			}

			miniSADSumSW(out1, out2, xOffSet, &miniSumSWRet, &OFRetSWRet);
			//		testMiniSADSumWrapperSW(refBlockColData, tagBlockColData, eventCnt, miniSumSW, OFRetSW);

			if (refBlockCol != 0 && tagBlockCol == 0)
			{
				cout << "Should not stop here, it is only a debug breakpoint." << endl;
			}
			cout  << "refBlockColSW is: " << hex <<  refBlockCol << endl;
			cout  << "tagBlockColSW is: " << hex <<  tagBlockCol << endl;
//			*refData++ = refBlockCol;
//			*tagData++ = tagBlockCol;
		}

		apUint17_t tmp1 = apUint17_t(xWr.to_int() + (yWr.to_int() << 8) + (1 << 16));
		ap_int<9> tmp2 = miniSumSWRet.range(8, 0);
		apUint6_t tmpOF = OFRetSWRet;
		ap_uint<32> output = (tmp2, (tmpOF, tmp1));
//		std :: cout << "tmp1 is "  << std::hex << tmp1 << std :: endl;
//		std :: cout << "tmp2 is "  << std::hex << tmp2 << std :: endl;
//		std :: cout << "output is "  << std::hex << output << std :: endl;
//		std :: cout << "eventSlice is "  << std::hex << output.to_int() << std :: endl;
		*eventSlice++ = output.to_int();
	}


	for (int16_t resetCnt = 0; resetCnt < 2048; resetCnt = resetCnt + 2)
	{
		resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL) * COMBINED_PIXELS, (sliceIdx_t)(idx + 3));
		resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL + 1) * COMBINED_PIXELS, (sliceIdx_t)(idx + 3));
	}
}

static uint16_t areaEventRegsSW[AREA_NUMBER][AREA_NUMBER];
static uint16_t areaEventThrSW = 1000;
static uint16_t OFRetRegsSW[2 * SEARCH_DISTANCE + 1][2 * SEARCH_DISTANCE + 1];


static void feedbackSW(apUint15_t miniSumRet, apUint6_t OFRet, apUint1_t rotateFlg, uint16_t *thrRet)
{
    if(miniSumRet <= 0x1ff && miniSumRet > 0 && OFRet != 0x3f)
    {
        uint16_t OFRetHistCnt = OFRetRegsSW[OFRet.range(2, 0)][OFRet.range(5, 3)];
        OFRetHistCnt = OFRetHistCnt + 1;
        OFRetRegsSW[OFRet.range(2, 0)][OFRet.range(5, 3)] = OFRetHistCnt;
    }

	if(rotateFlg)
	{
		uint16_t countSum = 0;
		uint16_t histCountSum = 0;
		uint16_t radiusSum =  0;
		uint16_t radiusCountSum =  0;

		for(int8_t OFRetHistX = -SEARCH_DISTANCE; OFRetHistX <= SEARCH_DISTANCE; OFRetHistX++)
		{
			for(int8_t OFRetHistY = -SEARCH_DISTANCE; OFRetHistY <= SEARCH_DISTANCE; OFRetHistY++)
			{
				uint16_t count = OFRetRegsSW[OFRetHistX+SEARCH_DISTANCE][OFRetHistY+SEARCH_DISTANCE];
				float radius = pow(OFRetHistX,  2) + pow(OFRetHistY,  2);
				countSum += count;
				radiusCountSum += radius * count;

				histCountSum += 1;
				radiusSum += radius;

				OFRetRegsSW[OFRetHistX+SEARCH_DISTANCE][OFRetHistY+SEARCH_DISTANCE] = 0;
			}
		}

		if (countSum >= 10)
		{
			float avgMatchDistance = (float)radiusCountSum / countSum;
			float avgTargetDistance = (float)radiusSum / histCountSum;

			if(avgMatchDistance > avgTargetDistance )
			{
//				areaEventThrSW -= areaEventThrSW * 3/64;
				if (areaEventThrSW <= 100)
				{
					areaEventThrSW = 100;
				}
				std::cout << "AreaEventThr is decreased. New areaEventThr from SW is: " << areaEventThrSW << std::endl;
			}
			else if (avgMatchDistance < avgTargetDistance)
			{

//				areaEventThrSW += areaEventThrSW *3/64;
				if (areaEventThrSW >= 1000)
				{
					areaEventThrSW = 1000;
				}
				std::cout << "AreaEventThr is increased. New areaEventThr from SW is: " << areaEventThrSW << std::endl;
			}
		}
	}


    *thrRet = areaEventThrSW;

}


uint32_t currentTs = 0, lastTs = 0;
void parseEventsSW(uint64_t * dataStream, int32_t eventsArraySize, int32_t *eventSlice)
{
//	glPLActiveSliceIdxSW--;
//	sliceIdx_t idx = glPLActiveSliceIdxSW;

//	cout << "Current Event packet's event number is: " << eventsArraySize << endl;
	for(int32_t i = 0; i < eventsArraySize; i++)
	{
		uint64_t tmp = *dataStream++;
		ap_uint<8> xWr, yWr;
		xWr = ((tmp) >> POLARITY_X_ADDR_SHIFT) & POLARITY_X_ADDR_MASK;
		yWr = ((tmp) >> POLARITY_Y_ADDR_SHIFT) & POLARITY_Y_ADDR_MASK;
		bool pol  = ((tmp) >> POLARITY_SHIFT) & POLARITY_MASK;
		uint64_t ts = tmp >> 32;

        /* These two values are only for debug and test */
        ap_uint<2> OFGT_scale = (tmp >> 14);
        ap_uint<3> OFGT_x = (tmp >> 26);
        ap_uint<3> OFGT_y = (tmp >> 29);
        ap_uint<6> OFGT = OFGT_y.concat(OFGT_x);

		ap_int<16> miniRet;
		ap_uint<6> OFRet;
		ap_uint<2> scaleRet;
		ap_int<16> miniRetScale0;
		ap_uint<6> OFRetScale0;
		ap_int<16> miniRetScale1;
		ap_uint<6> OFRetScale1;
		ap_int<16> miniRetScale2;
		ap_uint<6> OFRetScale2;

        uint16_t c = areaEventRegsSW[xWr/AREA_SIZE][yWr/AREA_SIZE];
        c = c + 1;
        areaEventRegsSW[xWr/AREA_SIZE][yWr/AREA_SIZE] = c;

        apUint1_t rotateFlg = 0;
        // The area threshold reached, rotate the slice index and clear the areaEventRegs.
        if(c >= areaEventThrSW)
        {
            glPLActiveSliceIdxSW--;
//            idx = glPLActiveSliceIdxSW;
            rotateFlg = 1;

            lastTs = currentTs;
            currentTs = ts;

            for(int r = 0; r < 1; r++)
            {
                cout << "Rotated successfully from SW!!!!" << endl;
                cout << "x is: " << xWr << "\t y is: " << yWr << "\t idx is: " << glPLActiveSliceIdxSW << endl;
                cout << "delataTs is: " << ((currentTs - lastTs) >> 9) << endl;
            }

            // Check the accumulation slice is clear or not
            for(int32_t xAddr = 0; xAddr < SLICE_WIDTH; xAddr++)
            {
                for(int32_t yAddr = 0; yAddr < SLICE_HEIGHT; yAddr = yAddr + COMBINED_PIXELS)
                {
                    if (slicesSW[glPLActiveSliceIdxSW][xAddr][yAddr/COMBINED_PIXELS] != 0)
                    {
                        for(int r = 0; r < 10; r++)
                        {
                            cout << "Ha! I caught you, the pixel which is not clear!" << endl;
                            cout << "x is: " << xAddr << "\t y is: " << yAddr << "\t idx is: " << glPLActiveSliceIdxSW << endl;
                        }
                    }
                }
            }

            for(int areaX = 0; areaX < AREA_NUMBER; areaX++)
            {
                for(int areaY = 0; areaY < AREA_NUMBER; areaY++)
                {
                    areaEventRegsSW[areaX][areaY] = 0;
                }
            }

           for (int16_t resetCnt = 0; resetCnt < 2048; resetCnt = resetCnt + 2)
           {
               resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL) * COMBINED_PIXELS, (sliceIdx_t)(glPLActiveSliceIdxSW + 3));
               resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL + 1) * COMBINED_PIXELS, (sliceIdx_t)(glPLActiveSliceIdxSW + 3));
           }

        }

		writePixSW(xWr, yWr, glPLActiveSliceIdxSW);

		resetPixSW(i/(PIXS_PER_COL), (i % (PIXS_PER_COL)) * COMBINED_PIXELS, (sliceIdx_t)(glPLActiveSliceIdxSW + 3));
//				resetPix(i/PIXS_PER_COL, (i % PIXS_PER_COL + 1) * COMBINED_PIXELS, (sliceIdx_t)(idx + 3));
//				resetPix(i, 64, (sliceIdx_t)(idx + 3));
//				resetPix(i, 96, (sliceIdx_t)(idx + 3));

//				resetPix(i, 128, (sliceIdx_t)(idx + 3));
//				resetPix(i, 160, (sliceIdx_t)(idx + 3));
//				resetPix(i, 192, (sliceIdx_t)(idx + 3));
//				resetPix(i, 224, (sliceIdx_t)(idx + 3));

		// We use stream to accumulate sum and obtain the minimum, so we don't need this global shift register.
//		for(int idx1 = 0; idx1 < BLOCK_SIZE; idx1++)
//		{
//			for(int idx2 = 0; idx2 < 2*SEARCH_DISTANCE + 1; idx2++)
//			{
//				localSumReg[idx1][idx2] = 0;
//			}
//		}
//		miniRetVal = 0x7fff;
//		minOFRet = ap_uint<6>(0xff);   // 0xff means the OF is invalid.

		// In software version, we initial miniSumTmp for every input, so we don't do it here.
//		initMiniSumLoop : for(int8_t j = 0; j <= 2*SEARCH_DISTANCE; j++)
//		{
//			miniSumTmp[j] = ap_int<16>(0);
//		}

        pix_t block1[BLOCK_SIZE][BLOCK_SIZE];
        pix_t block2[BLOCK_SIZE + 2 * SEARCH_DISTANCE][BLOCK_SIZE + 2 * SEARCH_DISTANCE];

        pix_t block1Scale1[BLOCK_SIZE][BLOCK_SIZE];
        pix_t block2Scale1[BLOCK_SIZE + 2 * SEARCH_DISTANCE][BLOCK_SIZE + 2 * SEARCH_DISTANCE];

        pix_t block1Scale2[BLOCK_SIZE][BLOCK_SIZE];
        pix_t block2Scale2[BLOCK_SIZE + 2 * SEARCH_DISTANCE][BLOCK_SIZE + 2 * SEARCH_DISTANCE];

		for(int8_t xOffset = 0; xOffset < BLOCK_SIZE + 2 * SEARCH_DISTANCE; xOffset++)
        {
            pix_t out1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

            pix_t out1Scale1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2Scale1[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

            pix_t out1Scale2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];
            pix_t out2Scale2[BLOCK_SIZE+ 2 * SEARCH_DISTANCE];

			readBlockColsSWScale0(xWr - BLOCK_SIZE/2 - SEARCH_DISTANCE + xOffset, yWr , (glPLActiveSliceIdxSW + 1), (glPLActiveSliceIdxSW + 2), out1, out2);
			readBlockColsSWScale1(xWr/2 - BLOCK_SIZE/2 - SEARCH_DISTANCE + xOffset, yWr/2 , (glPLActiveSliceIdxSW + 1), (glPLActiveSliceIdxSW + 2), out1Scale1, out2Scale1);
			readBlockColsSWScale2(xWr/4 - BLOCK_SIZE/2 - SEARCH_DISTANCE + xOffset, yWr/4 , (glPLActiveSliceIdxSW + 1), (glPLActiveSliceIdxSW + 2), out1Scale2, out2Scale2);

            for(int8_t yCopyOffset = 0; yCopyOffset < BLOCK_SIZE; yCopyOffset++)
            {
                if (xOffset >= SEARCH_DISTANCE && xOffset < BLOCK_SIZE + SEARCH_DISTANCE)
                {
                    block1[xOffset - SEARCH_DISTANCE][yCopyOffset] = out1[yCopyOffset + SEARCH_DISTANCE];
                    block1Scale1[xOffset - SEARCH_DISTANCE][yCopyOffset] = out1Scale1[yCopyOffset + SEARCH_DISTANCE];
                    block1Scale2[xOffset - SEARCH_DISTANCE][yCopyOffset] = out1Scale2[yCopyOffset + SEARCH_DISTANCE];
                }
            }

            for(int8_t yCopyOffset = 0; yCopyOffset < BLOCK_SIZE + 2 * SEARCH_DISTANCE; yCopyOffset++)
            {
                block2[xOffset][yCopyOffset] = out2[yCopyOffset];
                block2Scale1[xOffset][yCopyOffset] = out2Scale1[yCopyOffset];
                block2Scale2[xOffset][yCopyOffset] = out2Scale2[yCopyOffset];
            }
		}

        bool printBlocksEnable = false;
        miniBlockSADSW(block1Scale2, block2Scale2, printBlocksEnable, &miniRetScale2, &OFRetScale2);
        miniBlockSADSW(block1Scale1, block2Scale1, printBlocksEnable, &miniRetScale1, &OFRetScale1);
        miniBlockSADSW(block1, block2, printBlocksEnable, &miniRetScale0, &OFRetScale0);

        if(OFRetScale0 != 0x3f) miniRetScale0 = (miniRetScale0 << 4);
        if(OFRetScale1 != 0x3f) miniRetScale1 = (miniRetScale1 << 2);
        miniRet = miniRetScale2;
        OFRet = OFRetScale2; 
        scaleRet = 2;
        if(miniRetScale1 < miniRet)
        {
            miniRet = miniRetScale1;
            OFRet = OFRetScale1;
            scaleRet = 1;
        }
        if(miniRetScale0 < miniRet)
        {
            miniRet = miniRetScale0;
            OFRet = OFRetScale0;
            scaleRet = 0;
        }
//        // Remove outliers
//        int block1ZeroCnt = 0;
//        for(int8_t block1IdxX = 0; block1IdxX < BLOCK_SIZE; block1IdxX++)
//        {
//            for(int8_t block1IdxY = 0; block1IdxY < BLOCK_SIZE; block1IdxY++)
//            {
//                if(block1[block1IdxX][block1IdxY] == 0)
//                {
//                    block1ZeroCnt++;
//                }
//            }
//        }
//
//        if(block1ZeroCnt > BLOCK_SIZE * (BLOCK_SIZE - 1))
//        {
//            miniRet = 0x7fff;
//            OFRet = 0x3f;
//        }

        // check result, only check valid result
        if(OFRet != 0x3f)
        {
            if(!(xWr/4 - BLOCK_SIZE/2 - SEARCH_DISTANCE < 0 || xWr/4 + BLOCK_SIZE/2 + SEARCH_DISTANCE >= DVS_WIDTH/4
                   || yWr/4 - BLOCK_SIZE/2 - SEARCH_DISTANCE < 0 || yWr/4 + BLOCK_SIZE/2 + SEARCH_DISTANCE >= DVS_HEIGHT/4))
            {
                if(OFRet != OFGT || scaleRet != OFGT_scale)
                {
                    cout << "Found error at index: " << i << endl;
                    cout << "x is:  " << xWr << "\t y is: " << yWr << "\t ts is: " << ts << endl;
                }
            }
        }

		apUint17_t tmp1 = apUint17_t(xWr.to_int() + (yWr.to_int() << 8) + (pol << 16));
		ap_int<9> tmp2 = miniRet.range(8, 0);
        ap_uint<9> delataTs = ((currentTs - lastTs) >> 9); 
		apUint6_t tmpOF = OFRet;
		ap_uint<32> output = (delataTs, (tmpOF, tmp1));
		*eventSlice++ = output.to_int();

        /* -----------------Feedback part------------------------ */
		feedbackSW(miniRet, OFRet, rotateFlg, &areaEventThrSW);
	}

	resetLoop: for (int16_t resetCnt = 0; resetCnt < 2048; resetCnt = resetCnt + 2)
	{
		resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL) * COMBINED_PIXELS, (sliceIdx_t)(glPLActiveSliceIdxSW + 3));
		resetPixSW(resetCnt/PIXS_PER_COL, (resetCnt % PIXS_PER_COL + 1) * COMBINED_PIXELS, (sliceIdx_t)(glPLActiveSliceIdxSW + 3));
	}
}
