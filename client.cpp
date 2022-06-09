/**
 * OpenCV video streaming over TCP/IP
 * Client: Receives video from server and display it
 * by Steve Tuenkam
 * Edited by Kamil Kawka
 */

#include "opencv4/opencv2/opencv.hpp"
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <regex>

using namespace cv;


int         sockfd;
char*       serverIP;
int         serverPort;

void sig_int(int signo)
{
   printf("Received SIG = %d\n", signo);
   close(sockfd);
   exit(0);
}

int m_signal(int signum, void handler(int)){
    struct sigaction new_action, old_action;

    new_action.sa_handler = handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = SA_RESTART;

    if( sigaction (signum, &new_action, &old_action) < 0 ){
          fprintf(stderr,"sigaction error : %s\n", strerror(errno));
          return 1;
    }
    return 0;
}


int main(int argc, char** argv)
{

    if (argc != 3) {
           std::cerr << "Usage: cv_video_cli <serverIP> <serverPort> " << std::endl;
    }

    serverIP   = argv[1];
    serverPort = atoi(argv[2]);
    socklen_t len;
    struct timeval delay;

    std::regex e("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

     if (!(std::regex_match(serverIP, e))){
         perror("Regex of serverIP doesn't match");
         exit(1);
     }
    

    struct  sockaddr_in serverAddr;
    socklen_t           addrLen = sizeof(struct sockaddr_in);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket() failed" << std::endl;
    }


    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    serverAddr.sin_port = htons(serverPort);

    delay.tv_sec = 3; 
	delay.tv_usec = 1; 
	len = sizeof(delay);
	if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &delay, len) == -1 ){
		fprintf(stderr,"SO_RCVTIMEO setsockopt error : %s\n", strerror(errno));
		return -1;
	}

    if (connect(sockfd, (sockaddr*)&serverAddr, addrLen) < 0) {
        std::cerr << "connect() failed!" << std::endl;
    }

    
    m_signal(SIGINT, sig_int);

    //----------------------------------------------------------
    //OpenCV Code
    //----------------------------------------------------------

    Mat img;
    img = Mat::zeros(480 , 640, CV_8UC1);    
    int imgSize = img.total() * img.elemSize();
    uchar *iptr = img.data;
    int bytes = 0;
    int key;

    //make img continuos
    if ( ! img.isContinuous() ) { 
          img = img.clone();
    }
        
    std::cout << "Image Size:" << imgSize << std::endl;

    namedWindow("CV Video Client",1);

    while (key != 'q') {

        if ((bytes = recv(sockfd, iptr, imgSize, MSG_WAITALL)) == -1) {
            std::cerr << "recv failed, received bytes = " << bytes << std::endl;
        }
        
        cv::imshow("CV Video Client", img); 
      
        if (key = cv::waitKey(10) >= 0) break;
    }   

    close(sockfd);

    return 0;
}	