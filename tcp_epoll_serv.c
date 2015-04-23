/** 
 * @file:       tcp_epoll_serv.c
 * @brief:      simple tcp serv concurrent realized with epoll
 * @author:     retton
 * @date:       2015-04-23
 * @version:    V1.0.0
 * @note:       History:
 * @note:       <author><time><version><description>
 * @warning:    
 */
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_MAX_LISTEN   32
#define SERV_LISTEN_ADDR    "127.0.0.1"
#define SERV_LISTEN_PORT    "28091"

#define LOGERR(fmt, ...) do {\
    fprintf(stderr, "[%-20s%-4d]"fmt, __func__, __LINE__, ##__VA_ARGS__);\
    fflush(stderr);\
} while (0)

static int create_listener(const char *addr, const char *port)
{
    int sd;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = AI_PASSIVE;

    int ret = getaddrinfo(addr, port, &hints, &result);
    if (0 != ret) {
        LOGERR("getaddrinfo error!\n");
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        int len = 1;
        setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,
                (const void *)&len, sizeof(int));
        if (sd < 0) {
            continue;
        }

        if (0 == bind(sd, rp->ai_addr, rp->ai_addrlen)) {
            break;
        }

        close(sd);
    }

    if (rp == NULL) {
        LOGERR("could not bind!\n");
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);

    if (listen(sd, SERVER_MAX_LISTEN) != 0) {
        LOGERR("listen error[%m]\n");
        return -1;
    }

    return sd;
}

int main(void)
{
    int i = 0;
    int ret = -1;
    int epollfd = -1;
    int sockfd = -1;
    struct epoll_event ev;
    struct epoll_event events[SERVER_MAX_LISTEN];

init_epoll:
    if (0 > (epollfd = epoll_create1(0))) {
        LOGERR("epoll instance create error[%m]!\n");
        return -1;
    }

init_listen_socket:
    sleep(1);
    if (0 > (sockfd = create_listener(SERV_LISTEN_ADDR, SERV_LISTEN_PORT))) {
        LOGERR("retry create listen!!\n");
        goto init_listen_socket;
    }

    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events   = EPOLLIN;
    ev.data.fd  = sockfd;
    if (0 != epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) && errno != EEXIST) {
        LOGERR("epoll ctl error[%m]!\n");
        if (errno == EBADF || errno == EINVAL) {
            close(sockfd);
            close(epollfd);
            goto init_epoll;
        }
        return -1;
    }

    for (;;) {
        ret = epoll_wait(epollfd, events, SERVER_MAX_LISTEN, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                close(epollfd);
                close(sockfd);
                goto init_epoll;
            }
        }
        
        for (i = 0; i < ret; ++i) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                    || (!events[i].events & EPOLLIN))
            {
                close(events[i].data.fd);
                continue;
            } else if (sockfd == events[i].data.fd) {
                int connfd = -1;
                if (0 > (connfd = accept(sockfd, NULL, NULL))) {
                    LOGERR("accpet failed[%m]!\n");
                    close(sockfd);
                    goto init_listen_socket;
                }
                memset(&ev, 0, sizeof(struct epoll_event));
                ev.data.fd = connfd;
                ev.events = EPOLLIN;
                if (0 != epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev)) {
                    LOGERR("epoll add connfd error[%m]!\n");
                    continue;
                }
            } else if (events[i].events & EPOLLIN) {
                //process in fd data events[i].data.fd
            } else {
                //noting
            }
        }
    }

    return 0;
}
