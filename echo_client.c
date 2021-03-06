#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include "ikcp.h"

void millisecond_sleep(size_t n_millisecond)
{
    struct timespec sleepTime;
    struct timespec time_left_to_sleep;
    sleepTime.tv_sec = n_millisecond / 1000;
    sleepTime.tv_nsec = (n_millisecond % 1000) * 1000 * 1000;
    while( (sleepTime.tv_sec + sleepTime.tv_nsec) > 0 )
    {
        time_left_to_sleep.tv_sec = 0;
        time_left_to_sleep.tv_nsec = 0;
        int ret = nanosleep(&sleepTime, &time_left_to_sleep);
        if (ret < 0)
        {
            fprintf(stderr, "nanosleep error with err\n");
            //std::cerr << "nanosleep error with errno: " << errno << " " << strerror(errno) << std::endl;
        }
        sleepTime.tv_sec = time_left_to_sleep.tv_sec;
        sleepTime.tv_nsec = time_left_to_sleep.tv_nsec;
    }
}


/* get system time */
void itimeofday(long *sec, long *usec)
{
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
}

/* get clock in millisecond 64 */
uint64_t iclock64(void)
{
    long s, u;
    uint64_t value;
    itimeofday(&s, &u);
    value = ((uint64_t)s) * 1000 + (u / 1000);
    return value;
}


uint32_t iclock()
{
    return (uint32_t)(iclock64() & 0xfffffffful);
}

//设置非阻塞
static void setnonblocking(int sockfd) {
    int flag = fcntl(sockfd, F_GETFL, 0);
    if (flag < 0) {
        exit(EXIT_FAILURE);
        return;
    }
    if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
        exit(EXIT_FAILURE);
    }
}

#define MAXLINE 2048
#define LOCAL_IP "127.0.0.1"
#define LOCAL_PORT 8000

int fd = 0;

struct sockaddr *addr = NULL;
int udp_output(const char *buf, int len, ikcpcb *kcp, void *user){
    printf("udp_output\n");
    sendto(fd, buf, len, 0, addr, sizeof(*addr));
    return 0;
}

pthread_t thread;
pthread_cond_t cond;
pthread_mutex_t mutex;

int main(int argc, char *argv[]){
    struct sockaddr_in serveraddr, cliaddr;
    int sockfd, n;
    char recv_buf[MAXLINE];
    char send_buf[MAXLINE];
    int addr_len = sizeof(struct sockaddr_in);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        printf("socket failed\n");
        exit(EXIT_FAILURE);
    }

    setnonblocking(sockfd);

    fd = sockfd;

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(LOCAL_PORT);
    serveraddr.sin_addr.s_addr = inet_addr(LOCAL_IP);
    addr = &serveraddr;

    ikcpcb *kcp = ikcp_create(0x11223344, (void*)0);
    kcp->output = udp_output;

    if(1){
        strcpy(send_buf, "abcdefgh");
        n = ikcp_send(kcp, send_buf, strlen(send_buf));
        if (n == -1){
            printf("socket failed\n");
            exit(EXIT_FAILURE);
        }
    }

     while(1){
        uint32_t time1 = iclock();
        ikcp_update(kcp, time1);
        
        memset(recv_buf, 0, MAXLINE);
        //printf("before recvfrom\n");
        n = recvfrom(sockfd, recv_buf, MAXLINE, 0, (struct sockaddr *)&serveraddr, &addr_len);
        if (n > 0){
            printf("after recvfrom=%d\n",n);
            ikcp_input(kcp, recv_buf, n);
        }
        int msgLen = ikcp_peeksize(kcp);
        if (msgLen > 0){
            printf("msgLen=%d\n",msgLen);
            memset(send_buf, 0, MAXLINE);
            ikcp_recv(kcp, send_buf, msgLen);
            printf("client recv %s\n",send_buf);
        }
    }

    close(sockfd);
    return 0;
}