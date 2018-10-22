#include <libcaercpp/devices/davis.hpp>
#include <atomic>
#include <csignal>
#include "ABMOF/abmof.h"

#include <iostream>
#include <stdlib.h>
#include "sds_utils.h"

#ifdef __SDSCC__
#include "sds_lib.h"
#else
#define sds_alloc(x)(malloc(x))
#define sds_free(x)(free(x))
#endif

#define N 16
#define NUM_ITERATIONS N
typedef short data_t;

using namespace std;

static atomic_bool globalShutdown(false);

static void globalShutdownSignalHandler(int signal) {
	// Simply set the running flag to false on SIGTERM and SIGINT (CTRL+C) for global shutdown.
	if (signal == SIGTERM || signal == SIGINT) {
		globalShutdown.store(true);
	}
}

static void usbShutdownHandler(void *ptr) {
	(void) (ptr); // UNUSED.

	globalShutdown.store(true);
}

void mmult_sw( int *in1,   // Input matrix 1
               int *in2,   // Input matrix 2
               int *out,   // Output matrix (out = A x B)
               int dim     // Matrix size of one dimension
             )
{
    //Performs matrix multiplication out = in1 x in2
    for (int i = 0; i < dim; i++){
        for (int j = 0; j < dim; j++){
            for (int k = 0; k < dim; k++){
                out[i * dim + j] += in1[i * dim + k] * in2[k * dim  + j];
            }
        }
    }
}

int main(int argc, char *argv[]){
// Install signal handler for global shutdown.
#if defined(_WIN32)
	if (signal(SIGTERM, &globalShutdownSignalHandler) == SIG_ERR) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGTERM. Error: %d.", errno);
		return (EXIT_FAILURE);
	}

	if (signal(SIGINT, &globalShutdownSignalHandler) == SIG_ERR) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGINT. Error: %d.", errno);
		return (EXIT_FAILURE);
	}
#else
	struct sigaction shutdownAction;

	shutdownAction.sa_handler = &globalShutdownSignalHandler;
	shutdownAction.sa_flags   = 0;
	sigemptyset(&shutdownAction.sa_mask);
	sigaddset(&shutdownAction.sa_mask, SIGTERM);
	sigaddset(&shutdownAction.sa_mask, SIGINT);

	if (sigaction(SIGTERM, &shutdownAction, NULL) == -1) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGTERM. Error: %d.", errno);
		return (EXIT_FAILURE);
	}

	if (sigaction(SIGINT, &shutdownAction, NULL) == -1) {
		libcaer::log::log(libcaer::log::logLevel::CRITICAL, "ShutdownAction",
			"Failed to set signal handler for SIGINT. Error: %d.", errno);
		return (EXIT_FAILURE);
	}
#endif

//	int dim = DATA_SIZE;
//    size_t matrix_size_bytes = sizeof(int) * DATA_SIZE * DATA_SIZE;
//
//    //Allocate memory:
//    int *in1 = (int *) sds_alloc(matrix_size_bytes);
//    int *in2 = (int *) sds_alloc(matrix_size_bytes);
//    int *hw_result = (int *) sds_alloc(matrix_size_bytes);
//    int *sw_result = (int *) malloc(matrix_size_bytes);
//
//    if( (in1 == NULL) || (in2 == NULL) || (sw_result == NULL) || (hw_result == NULL) )
//        {
//            std::cout << "TEST FAILED : Failed to allocate memory" << std::endl;
//            return -1;
//        }
//
//    //Create test data
//    for (int i = 0; i < dim * dim; i++) {
//        in1[i] = rand() % dim;
//        in2[i] = rand() % dim;
//        sw_result[i] = 0;
//        hw_result[i] = 0;
//     }
//
//    sds_utils::perf_counter hw_ctr, sw_ctr;
//
//    sw_ctr.start();
//    //Launch the software solution
//    mmult_sw( in1, in2, sw_result, dim);
//    sw_ctr.stop();
//
//    hw_ctr.start();
//    //Launch the Hardware solution
//    mmult_zero_copy( in1, in2, hw_result, dim);
//    hw_ctr.stop();
//
//    //Compare the results of hardware to the CPU
//    bool match = true;
//    for(int i=0; i< dim * dim; i++)
//    {
//        if( sw_result[i] != hw_result[i] )
//        {
//            std::cout << "Results Mismatch on " << "Row:" << i/dim << "Col:" << i - (i/dim)*dim << std::endl;
//            std::cout << "CPU output:" << sw_result[i] <<"\t Hardware output:" << hw_result[i] << std::endl;
//            match = false;
//            break;
//        }
//    }
//
//    uint64_t sw_cycles = sw_ctr.avg_cpu_cycles();
//    uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
//    double speedup = (double)sw_cycles/ (double)hw_cycles;
//
//    std::cout << "Number of CPU cycles running application in software:" << sw_cycles << std::endl;
//    std::cout << "Number of CPU cycles running application in hardware:" << hw_cycles << std::endl;
//    std::cout << "Speed up: " << speedup << std::endl;
//    std::cout << "Note: Speed up is meaningful for real hardware execution only, not for emulation." << std::endl;
//
//    //Release Memory
//    sds_free(in1);
//    sds_free(in2);
//    sds_free(hw_result);
//    free(sw_result);
//
//    std::cout << " TEST " << (match? "PASSED": "FAILED") << std::endl;




//	/********  OF hw part ******************/
//	cv::Mat frame0, frame1;
//	cv::Mat frame_out;
//
//	if (argc != 3) {
//		std::cout << "Usage incorrect. Correct usage: ./exe <current frame> <next frame>" << std::endl;
//		// return -1;
//	}
//	frame0 = cv::imread(argv[1],0);
//	frame1 = cv::imread(argv[2],0);
//
//	if (frame0.empty() || frame1.empty()) {
//		std::cout << "input files not found!" << std::endl;
//		return -1;
//	}
//	frame_out.create(frame0.rows, frame0.cols, CV_8UC4);
//	int cnt = 0;
//	unsigned char p1, p2, p3, p4;
//	unsigned int pix =0;
//
//	char out_string[200];
//	static xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> buf0(frame0.rows,frame0.cols);
//	static xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> buf1(frame0.rows,frame0.cols);
//	static xf::Mat<XF_32FC1,MAX_HEIGHT, MAX_WIDTH, NPPC> flowx(frame0.rows,frame0.cols);
//	static xf::Mat<XF_32FC1,MAX_HEIGHT, MAX_WIDTH, NPPC> flowy(frame0.rows,frame0.cols);
//
//	buf0.copyTo(frame0.data);
//	buf1.copyTo(frame1.data);
//	//buf0 = xf::imread<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(argv[1], 0);
//	//buf1 = xf::imread<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(argv[2], 0);
//
//	#if __SDSCC__
//		hw_ctr.start();
//	#endif
//		dense_non_pyr_of_accel(buf0, buf1, flowx, flowy);
//	#if __SDSCC__
//		hw_ctr.stop();
//		hw_cycles = hw_ctr.avg_cpu_cycles();
//	#endif
//
//	/* getting the flow vectors from hardware and colorcoding the vectors on a canvas of the size of the input*/
//	float *flowx_copy;
//	float *flowy_copy;
//	flowx_copy = (float *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(float)));
//	if(flowx_copy==NULL){
//		fprintf(stderr,"\nFailed to allocate memory for flowx_copy\n");
//	}
//	flowy_copy = (float *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(float)));
//	if(flowy_copy==NULL){
//		fprintf(stderr,"\nFailed to allocate memory for flowy_copy\n");
//	}
//	unsigned int *outputBuffer;
//	outputBuffer = (unsigned int *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(unsigned int)));
//	if(outputBuffer==NULL){
//		fprintf(stderr,"\nFailed to allocate memory for outputBuffer\n");
//	}
//
//	flowx_copy = (float *)flowx.copyFrom();
//	flowy_copy = (float *)flowy.copyFrom();
//	hls::stream <rgba_t> out_pix("Color pixel");
//	xf::getOutPix<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(flowx_copy,flowy_copy,frame1.data,out_pix,frame0.rows,frame0.cols,frame0.cols*frame0.rows);
//	xf::writeMatRowsRGBA<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1, KMED>(out_pix, outputBuffer,frame0.rows,frame0.cols,frame0.cols*frame0.rows);
//
//	rgba_t *outbuf_copy;
//	for(int i=0;i<frame0.rows;i++){
//		for(int j=0;j<frame0.cols;j++){
//			outbuf_copy = (rgba_t *) (outputBuffer + i*(frame0.cols) + j);
//			p1 = outbuf_copy->r;
//			p2 = outbuf_copy->g;
//			p3 = outbuf_copy->b;
//			p4 = outbuf_copy->a;
//			pix = ((unsigned int)p4 << 24) | ((unsigned int)p3 << 16) | ((unsigned int)p2 << 8) | (unsigned int)p1 ;
//			frame_out.at<unsigned int>(i,j) = pix;
//		}
//	}
//
//	sprintf(out_string,"out_%d.png", cnt);
//	cv::imwrite(out_string,frame_out);




	/************** libcaer part ***********************/

    int socketPort = 4097, eventThreshold = 50000, packetInterval = 10000, socketType = 0;  // Default value
    if (argc == 2) socketPort = atoi(argv[1]);
    if (argc == 3)
    {
    	socketPort = atoi(argv[1]);
    	eventThreshold = atoi(argv[2]);
    }
    if (argc == 4)
    {
    	socketPort = atoi(argv[1]);
    	eventThreshold = atoi(argv[2]);
    	packetInterval = atoi(argv[3]);
    }
    if (argc == 5)
    {
    	socketPort = atoi(argv[1]);
    	eventThreshold = atoi(argv[2]);
    	packetInterval = atoi(argv[3]);
    	socketType = atoi(argv[4]);
    }
    int remoteSocket;

//    // Open a DAVIS, give it a device ID of 1, and don't care about USB bus or SN restrictions.
//	libcaer::devices::davis davisHandle = libcaer::devices::davis(1);
//
//	// Let's take a look at the information we have on the device.
//	struct caer_davis_info davis_info = davisHandle.infoGet();
//
//	printf("%s --- ID: %d, Master: %d, DVS X: %d, DVS Y: %d, Logic: %d.\n", davis_info.deviceString,
//		davis_info.deviceID, davis_info.deviceIsMaster, davis_info.dvsSizeX, davis_info.dvsSizeY,
//		davis_info.logicVersion);
//
//	// Send the default configuration before using the device.
//	// No configuration is sent automatically!
//	davisHandle.sendDefaultConfig();
//
//	// Tweak some biases, to increase bandwidth in this case.
//	struct caer_bias_coarsefine coarseFineBias;
//
//	coarseFineBias.coarseValue        = 2;
//	coarseFineBias.fineValue          = 116;
//	coarseFineBias.enabled            = true;
//	coarseFineBias.sexN               = false;
//	coarseFineBias.typeNormal         = true;
//	coarseFineBias.currentLevelNormal = true;
//
//	davisHandle.configSet(DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRBP, caerBiasCoarseFineGenerate(coarseFineBias));
//
//	coarseFineBias.coarseValue        = 1;
//	coarseFineBias.fineValue          = 33;
//	coarseFineBias.enabled            = true;
//	coarseFineBias.sexN               = false;
//	coarseFineBias.typeNormal         = true;
//	coarseFineBias.currentLevelNormal = true;
//
//	davisHandle.configSet(DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRSFBP, caerBiasCoarseFineGenerate(coarseFineBias));
//
//	// Let's verify they really changed!
//	uint32_t prBias   = davisHandle.configGet(DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRBP);
//	uint32_t prsfBias = davisHandle.configGet(DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRSFBP);
//
//	printf("New bias values --- PR-coarse: %d, PR-fine: %d, PRSF-coarse: %d, PRSF-fine: %d.\n",
//		caerBiasCoarseFineParse(prBias).coarseValue, caerBiasCoarseFineParse(prBias).fineValue,
//		caerBiasCoarseFineParse(prsfBias).coarseValue, caerBiasCoarseFineParse(prsfBias).fineValue);
//
//	// Now let's get start getting some data from the device. We just loop in blocking mode,
//	// no notification needed regarding new events. The shutdown notification, for example if
//	// the device is disconnected, should be listened to.
//	davisHandle.dataStart(nullptr, nullptr, nullptr, &usbShutdownHandler, nullptr);
//
//	// Let's turn on blocking data-get mode to avoid wasting resources.
//	davisHandle.configSet(CAER_HOST_CONFIG_DATAEXCHANGE, CAER_HOST_CONFIG_DATAEXCHANGE_BLOCKING, true);
//
//	// Set data exchange buffer size
//	// davisHandle.configSet(CAER_HOST_CONFIG_DATAEXCHANGE, CAER_HOST_CONFIG_DATAEXCHANGE_BUFFER_SIZE, 10);
//	uint32_t bufferSize = davisHandle.configGet(CAER_HOST_CONFIG_DATAEXCHANGE, CAER_HOST_CONFIG_DATAEXCHANGE_BUFFER_SIZE);
//
//	// Set log level
//	davisHandle.configSet(CAER_HOST_CONFIG_LOG, CAER_HOST_CONFIG_LOG_LEVEL, 5);
//
//	// Set time interval
//	davisHandle.configSet(CAER_HOST_CONFIG_PACKETS, CAER_HOST_CONFIG_PACKETS_MAX_CONTAINER_INTERVAL, packetInterval);

	while (!globalShutdown.load(memory_order_relaxed)) {

        remoteSocket = abmof(socketPort, eventThreshold, socketType);

//		std::unique_ptr<libcaer::events::EventPacketContainer> packetContainer = davisHandle.dataGet();
//		if (packetContainer == nullptr) {
//			continue; // Skip if nothing there.
//		}
//
//		printf("\nGot event container with %d packets (allocated).\n", packetContainer->size());
//
//		for (auto &packet : *packetContainer) {
//			if (packet == nullptr) {
//				printf("Packet is empty (not present).\n");
//				continue; // Skip if nothing there.
//			}
//
//			printf("Packet of type %d -> %d events, %d capacity.\n", packet->getEventType(), packet->getEventNumber(),
//				packet->getEventCapacity());
//
//			if (packet->getEventType() == POLARITY_EVENT) {
//				std::shared_ptr<const libcaer::events::PolarityEventPacket> polarity
//					= std::static_pointer_cast<libcaer::events::PolarityEventPacket>(packet);
//
//				// Get full timestamp and addresses of first event.
////				CAER_POLARITY_ITERATOR_VALID_START(polarity)
////				uint16_t x        = caerPolarityEventGetX(caerPolarityIteratorElement);
////				uint16_t y        = caerPolarityEventGetY(caerPolarityIteratorElement);
////				bool pol          = caerPolarityEventGetPolarity(caerPolarityIteratorElement);
////				int64_t ts        = caerPolarityEventGetTimestamp64(caerPolarityIteratorElement, polarity);
//
//				remoteSocket = abmof(polarity, socketPort, eventThreshold, socketType);
//
//				// printf("First polarity event - ts: %d, x: %d, y: %d, pol: %d.\n", ts, x, y, pol);
////				CAER_POLARITY_ITERATOR_VALID_END
//			}
//
//			if (packet->getEventType() == FRAME_EVENT) {
//				std::shared_ptr<const libcaer::events::FrameEventPacket> frame
//					= std::static_pointer_cast<libcaer::events::FrameEventPacket>(packet);
//
//				// Get full timestamp, and sum all pixels of first frame event.
//				const libcaer::events::FrameEvent &firstEvent = (*frame)[0];
//
//				int32_t ts   = firstEvent.getTimestamp();
//				uint64_t sum = 0;
//
//				for (int32_t y = 0; y < firstEvent.getLengthY(); y++) {
//					for (int32_t x = 0; x < firstEvent.getLengthX(); x++) {
//						sum += firstEvent.getPixel(x, y);
//					}
//				}
//
//				printf("First frame event - ts: %d, sum: %" PRIu64 ".\n", ts, sum);
//			}
//		}
	}

//	davisHandle.dataStop();

	// Close automatically done by destructor.

    close(remoteSocket);   // close socket;
	printf("Shutdown successful.\n");

	return (EXIT_SUCCESS);
}
