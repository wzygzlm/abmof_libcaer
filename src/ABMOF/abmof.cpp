#include "abmof.h"
#include "abmof_hw_accel.h"

// xfopencv
// only used in SDx environment
// #include "xf_headers.h"
// #include "xf_dense_npyr_optical_flow_config.h"


// standard opencv, used in standard opencv environment
#include "opencv2/opencv.hpp"

#include <math.h>

// #include "sds_utils.h"

// TODO, hardcode now, should adapt to the real chip size.  uchar slices[SLICES_NUMBER][DVS_HEIGHT][DVS_WIDTH]; 
// Current slice index
static int8_t currentIdx = 0;

static uint64_t imgNum = 0;
static bool initSocketFlg = false;
static uint16_t retSocket;

// static outputDataElement_t *eventSlice = (outputDataElement_t *)sds_alloc(DVS_HEIGHT * DVS_WIDTH);
static int32_t eventSliceSW[DVS_HEIGHT * DVS_WIDTH];

// To trigger the tcp to send event slice
static bool sendFlg = false;

static void *displayTCP(void *);

static void *displayUDP(void *);

// TCP server
int init_socket_TCP(int port)
{

    //--------------------------------------------------------
    //networking stuff: socket, bind, listen
    //--------------------------------------------------------
    int                 localSocket,
                        remoteSocket;

    struct  sockaddr_in localAddr,
                        remoteAddr;
    pthread_t thread_id;


    int addrLen = sizeof(struct sockaddr_in);


    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    if (localSocket == -1){
         perror("socket() call failed!!");
    }

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons( port );

    if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
         perror("Can't bind() socket");
         exit(1);
    }

    //Listening
    listen(localSocket , 3);

    std::cout <<  "Waiting for connections...\n"
              <<  "Server Port:" << port << std::endl;

    //accept connection from an incoming client
    // while(1){
    //if (remoteSocket < 0) {
    //    perror("accept failed!");
    //    exit(1);
    //}

     remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
      //std::cout << remoteSocket<< "32"<< std::endl;
    if (remoteSocket < 0) {
        perror("accept failed!");
        exit(1);
    }
    std::cout << "Connection accepted" << std::endl;
     pthread_create(&thread_id,NULL,displayTCP,&remoteSocket);
     pthread_setname_np(thread_id, "SliceDisplay");
     sleep(5);

     //pthread_join(thread_id,NULL);

    // }
    //pthread_join(thread_id,NULL);
    //close(remoteSocket);

    return remoteSocket;
}

static void *displayTCP(void *ptr)
{
    int socket = *(int *)ptr;
    //OpenCV Code
    //----------------------------------------------------------

    cv::Mat img = cv::Mat(DVS_HEIGHT, DVS_WIDTH, CV_8UC1, eventSliceSW);
     //make it continuous
    if (!img.isContinuous()) {
        img = img.clone();
    }

    int imgSize = img.total() * img.elemSize();
    int bytes = 0;
    int key;


    std::cout << "Image Size:" << imgSize << std::endl;

    while(1) {
        if(sendFlg)
        {
            /* get a frame from camera */
            img = cv::Mat(DVS_HEIGHT, DVS_WIDTH, CV_8UC1, eventSliceSW);
            double maxIntensity;
            // cv::minMaxLoc(img, NULL, &maxIntensity);
            // std::cout<<"max value is "<<maxIntensity<<std::endl;
            //do video processing here
            // cvtColor(img, imgGray, CV_BGR2GRAY);

            //send processed image
            if ((bytes = send(socket, img.data, imgSize, 0)) < 0){
                std::cerr << "bytes = " << bytes << std::endl;
                break;
            }
            // sendFlg = false;
        }
    }

}


// UDP server
int init_socket_UDP(int port)
{

    //--------------------------------------------------------
    //networking stuff: socket, bind, listen
    //--------------------------------------------------------
    int                 localSocket,
                        remoteSocket;

    struct  sockaddr_in localAddr,
                        remoteAddr;
    pthread_t thread_id;


    int addrLen = sizeof(struct sockaddr_in);


    localSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (localSocket == -1){
         perror("socket() call failed!!");
    }

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons( port );

    if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
         perror("Can't bind() socket");
         exit(1);
    }

    //Listening
//    listen(localSocket , 3);
//
//    std::cout <<  "Waiting for connections...\n"
//              <<  "Server Port:" << port << std::endl;

    //accept connection from an incoming client
    // while(1){
    //if (remoteSocket < 0) {
    //    perror("accept failed!");
    //    exit(1);
    //}

     // UDP doesn't have accept, we only use localSocket to replace it.
     // remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen);
    remoteSocket = localSocket;
      //std::cout << remoteSocket<< "32"<< std::endl;
    if (remoteSocket < 0) {
        perror("accept failed!");
        exit(1);
    }
    std::cout << "Connection accepted" << std::endl;
     pthread_create(&thread_id,NULL,displayUDP,(void *)&localSocket);
     pthread_setname_np(thread_id, "SliceDisplay");
     sleep(5);

     //pthread_join(thread_id,NULL);

    // }
    //pthread_join(thread_id,NULL);
    //close(remoteSocket);

    return remoteSocket;
}

static void *displayUDP(void *ptr)
{
    int socket = *(int *)ptr;
    struct  sockaddr_in remoteAddr;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    char buf[10];
    int recvLen;

    std::cout <<  "Waiting for connections...\n" << std::endl;
    //try to receive some data, this is a blocking call
    if ((recvLen = recvfrom(socket, buf, 10, 0, (struct sockaddr *) &remoteAddr, &addrLen)) == -1)
    {
        std::cerr << "bytes = " << recvLen << std::endl;
        return NULL;
    }

    //print details of the client/peer and the data received
    printf("Received packet from %s:%d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));
    printf("Data: %s\n" , buf);

    //OpenCV Code
    //----------------------------------------------------------


    cv::Mat img = cv::Mat(DVS_HEIGHT, DVS_WIDTH, CV_8UC1, eventSliceSW);
     //make it continuous
    if (!img.isContinuous()) {
        img = img.clone();
    }

    int imgSize = img.total() * img.elemSize();
    int bytes = 0;
    int key;

    std::cout << "Image Size:" << imgSize << std::endl;

    while(1) {
        if(sendFlg)
        {
            /* get a frame from camera */
            img = cv::Mat(DVS_HEIGHT, DVS_WIDTH, CV_8UC1, eventSliceSW);
            double maxIntensity;
            // cv::minMaxLoc(img, NULL, &maxIntensity);
            // std::cout<<"max value is "<<maxIntensity<<std::endl;
            //do video processing here
            // cvtColor(img, imgGray, CV_BGR2GRAY);

            //send processed image
            if ((bytes = sendto(socket, (void *)(eventSliceSW), DVS_HEIGHT * DVS_WIDTH, 0, (struct sockaddr*)&remoteAddr, addrLen)) < 0){
                std::cerr << "bytes = " << bytes << std::endl;
                break;
            }
            // sendFlg = false;
        }
    }

}


//void saveImg(char img[DVS_WIDTH][DVS_HEIGHT], long cnt)
//{
//	cv::Mat frame_out = cv::Mat(DVS_WIDTH, DVS_HEIGHT, XF_8UC1, img);
//
//	char out_string[200];
//	sprintf(out_string,"out_%ld.png", cnt);
//
//	cv::imwrite(out_string,frame_out);
//}

void sendEventSlice()
{
    sendFlg = true;
}


int abmof(std::shared_ptr<const libcaer::events::PolarityEventPacket> polarityPkt, int port, int eventThreshold, int socketType)
{
	if (!initSocketFlg)
	{
		if (socketType == 0)     //0 : UDP
		{
			retSocket = init_socket_UDP(port);
		}
		else
		{
			retSocket = init_socket_TCP(port);
		}
		initSocketFlg = true;
	}

	// resetSlices();   // Clear slices before a new packet come in

	imgNum++;

	// Get full timestamp and addresses of first event.
	const libcaer::events::PolarityEvent &firstEvent = (*polarityPkt)[0];

	int32_t ts = firstEvent.getTimestamp();
	uint16_t x = firstEvent.getX();
	uint16_t y = firstEvent.getY();
	bool pol   = firstEvent.getPolarity();

	int32_t eventsArraySize = (*polarityPkt).getEventNumber();
	int32_t eventPerSize = (*polarityPkt).getEventSize();

    // Make suer sds_alloc allocate a right memory for eventSlice.
//	if(eventSlice == NULL)
//	{
//		eventSlice = (outputDataElement_t *)sds_alloc(DVS_HEIGHT * DVS_WIDTH);
//		return retSocket;
//	}
	if(eventsArraySize >= eventThreshold)
	{
		eventsArraySize = eventThreshold;
	}

	// Currently we need to make eventSlice a static pointer, otherwise the display thread may have
	// competence with this main thread. To achieve this, we must specify the size of eventSlice.
	// For software, this might not be an issue since eventSlice is a FIFO which doesn't have address
	// when implemented on hardware.
	if(eventsArraySize >= DVS_HEIGHT * DVS_WIDTH / 4)
	{
		eventsArraySize = DVS_HEIGHT * DVS_WIDTH / 4;
	}

	uint64_t * data = (uint64_t *)malloc(eventsArraySize * eventPerSize);
	memcpy(data, (void *)&(firstEvent.data), eventsArraySize * eventPerSize);
//    sds_utils::perf_counter hw_ctr, sw_ctr;
//
//    sw_ctr.start();
    parseEventsSW(data, eventsArraySize, eventSliceSW);
//    sw_ctr.stop();
//
//    hw_ctr.start();
//    // reset the eventSlice
//    memset((char *) eventSlice, 0, DVS_HEIGHT * DVS_WIDTH);
//
//	parseEvents(data, eventsArraySize, eventSlice);
//	hw_ctr.stop();
//
//	uint64_t sw_cycles = sw_ctr.avg_cpu_cycles();
//    uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();

//    std::cout << "Number of CPU cycles running application in software: "
//                << sw_cycles << std::endl;
//
//    std::cout << "Number of CPU cycles running application in hardware: "
//                << hw_cycles << std::endl;

//    int cmpRet;
//
//    cmpRet = memcmp( eventSliceSW, eventSlice, eventsArraySize);
//
//    if (cmpRet != 0) std::cout << "Test failed" << std::endl;

//    sds_free(data);
	sendEventSlice();

//	int i = 0;
//	for (auto &tmpEvent : *polarityPkt)
//	{
//		i++;
//		int32_t ts = tmpEvent.getTimestamp();
//		uint16_t x = tmpEvent.getX();
//		uint16_t y = tmpEvent.getY();
//		bool pol   = tmpEvent.getPolarity();
//
//		accumulate(x, y, pol, ts);
//
//		// printf("Current event - ts: %d, x: %d, y: %d, pol: %d.\n", ts, x, y, pol);
//	}
//	printf("eventSize is %d, eventCap is %d, i is %d", polarity->getEventNumber(), polarity->getEventCapacity(), i);

	return retSocket;
}
