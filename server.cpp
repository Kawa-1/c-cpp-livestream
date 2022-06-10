/**
 * OpenCV video streaming over TCP/IP
 * Server: Captures video from a webcam and send it to a client
 * by Isaac Maia
 * Edited by Marcin Bednarz
 */

#include "opencv2/opencv.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define MAXEVENTS 10
#define LISTENQ 10

using namespace cv;

static int epollfd;
static int localSocket;

void display(void *);

int capDev = 0;

VideoCapture cap(capDev); // open the default camera

int create_socket()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket() call failed!!");
        exit(1);
    }

    int sndbuf = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sndbuf, sizeof(sndbuf)) < 0)
    {
        perror("SO_REUSEADDR setsockopt error :)");
    }

    
    return sock;
}

int main(int argc, char **argv)
{
    int remoteSocket,
        port = 4097;
    int nready;

    struct sockaddr_in localAddr,
        remoteAddr;
    struct epoll_event events[MAXEVENTS];
    struct epoll_event ev;

    int addrLen = sizeof(struct sockaddr_in);

    signal(SIGPIPE, SIG_IGN);

    if ((argc > 1) && (strcmp(argv[1], "-h") == 0))
    {
        std::cerr << "usage: ./server [port] [capture device]\n"
                  << "port           : socket port (4097 default)\n"
                  << "capture device : (0 default)\n"
                  << std::endl;

        exit(1);
    }

    if (argc == 2)
        port = atoi(argv[1]);

    localSocket = create_socket();
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(port);

    if (bind(localSocket, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0)
    {
        perror("Can't bind() socket");
        exit(1);
    }

    if ((epollfd = epoll_create(MAXEVENTS)) < 0)
    {
        perror("Epoll create error:");
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = localSocket;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, localSocket, &ev) == -1)
    {
        perror("Epol ctl error: ");
        return 1;
    }

    

    // Listening
    listen(localSocket, LISTENQ);

    std::cout << "Waiting for connections...\n"
              << "Server Port:" << port << std::endl;

    // accept connection from an incoming client
    while (1)
    {
        if ((nready = epoll_wait(epollfd, events, MAXEVENTS, -1)) == -1)
        {
            perror("Epoll listen wait error: ");
            return 1;
        }

        for (int i=0; i<nready; i++)
        {
            if (events[i].data.fd == localSocket)
            {   
                printf("Local socket: %d\n", events[i].data.fd);
                if ((remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t *)&addrLen)) < 0)
                {
                    perror("Accept error: ");
                }
                else 
                {
                    ev.events = EPOLLOUT;
                    ev.data.fd = remoteSocket;
                    printf("Remote socket fd after accept: %d\n", remoteSocket);
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, remoteSocket, &ev) == -1)
                    {
                        perror("Epol ctl error after accept: ");
                        continue;
                    }
                }
            }
            else
            {
                remoteSocket = events[i].data.fd;
                printf("FD: %d\n", remoteSocket);
                display(&remoteSocket);
            }
        }
    }

    printf("Jak to tutaj doszlo?????\n");
    close(localSocket);
    return 0;
}

void display(void *ptr)
{
    int socket = *(int *)ptr;
    printf("Display FD: %d \n", socket);
    // OpenCV Code
    //----------------------------------------------------------

    Mat img, imgGray;
    img = Mat::zeros(480, 640, CV_8UC1);
    // make it continuous
    if (!img.isContinuous())
    {
        img = img.clone();
    }

    int imgSize = img.total() * img.elemSize();
    int bytes = 0;

    // make img continuos
    if (!img.isContinuous())
    {
        img = img.clone();
        imgGray = img.clone();
    }

    std::cout << "Image Size:" << imgSize << std::endl;

    /* get a frame from camera */
    cap >> img;

    // do video processing here
    cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);

    // send processed image
    if ((bytes = send(socket, imgGray.data, imgSize, 0)) < 0)
    {
        //std::cerr << "bytes = " << bytes << std::endl;
        //perror("Error send: ");
        close(socket);
    }
}
