/* Base version was generated with:
        ANHTROPIC Claude 4.6 Sonnet
    prompt:
    "can you write a minimal tcp non-block echo server for linux in C"
*/
/*
 * minimal non-blocking TCP echo server (Linux / epoll, edge-triggered)
 * build: gcc -O2 -o echoserver echoserver.c
 * test:  nc localhost 8080
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#ifdef KCUT_TCP
// uncomment next line or pass -D EVACUATE at compile to turn on evacution
#define EVACUATE
#include "kcut_tcpmsg.h"
#endif

#define PORT       8080
#define BACKLOG    128
#define MAX_EVENTS 64
#define BUF_SIZE   4096

static int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void close_fd(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);   /* optional but clean */
    close(fd);
}

int main(void)
{
    /* ---- listening socket ---- */
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) { perror("socket"); return 1; }

    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };
    if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("bind"); return 1;
    }
    if (listen(lfd, BACKLOG) == -1) { perror("listen"); return 1; }
    if (set_nonblock(lfd) == -1)    { perror("fcntl");  return 1; }

    /* ---- epoll instance ---- */
    int epfd = epoll_create1(0);
    if (epfd == -1) { perror("epoll_create1"); return 1; }

    struct epoll_event ev = { .events = EPOLLIN, .data.fd = lfd };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) == -1) {
        perror("epoll_ctl"); return 1;
    }

    printf("echo server listening on port %d\n", PORT);

    struct epoll_event events[MAX_EVENTS];
    char buf[BUF_SIZE];

#ifdef KCUT_TCP
    kcut_init();
#endif
    
    for (;;) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd     = events[i].data.fd;
            uint32_t e = events[i].events;

            /* ---- new connection ---- */
            if (fd == lfd) {
                for (;;) {
                    struct sockaddr_in ca;
                    socklen_t clen = sizeof ca;
                    int cfd = accept(lfd, (struct sockaddr *)&ca, &clen);
                    if (cfd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;          /* drained */
                        perror("accept");
                        break;
                    }
                    if (set_nonblock(cfd) == -1) {
                        perror("fcntl"); close(cfd); continue;
                    }
                    printf("connect  fd=%-4d  %s:%d\n",
                           cfd,
                           inet_ntoa(ca.sin_addr),
                           ntohs(ca.sin_port));

                    /* edge-triggered: we must drain the fd ourselves */
                    ev.events  = EPOLLIN | EPOLLET;
                    ev.data.fd = cfd;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
                        perror("epoll_ctl"); close(cfd);
                    }
                }
                continue;
            }

            /* ---- error / hangup ---- */
            if (e & (EPOLLERR | EPOLLHUP)) {
                printf("disconnect fd=%d\n", fd);
                close_fd(epfd, fd);
                continue;
            }

            /* ---- data ready ---- */
            if (e & EPOLLIN) {
                int closed = 0;

                /* drain the socket completely (required with EPOLLET) */
                for (;;) {
#ifndef KCUT_TCP
                    ssize_t nr = read(fd, buf, sizeof buf);
#else
		    ssize_t nr = kcut_tcp_read(fd, buf, sizeof buf);
#endif
                    if (nr == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;           /* fully drained */
                        perror("read");
                        closed = 1;
                        break;
                    }
                    if (nr == 0) {           /* peer closed */
                        printf("close    fd=%d\n", fd);
                        closed = 1;
                        break;
                    }

                    /* echo: write everything back */
                    ssize_t sent = 0;
                    while (sent < nr) {
#ifndef KCUT_TCP		      
                        ssize_t nw = write(fd, buf + sent, nr - sent);
#else
			ssize_t nw = kcut_tcp_write(fd, buf + sent, nr - sent);
#endif
                        if (nw == -1) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                /* NOTE: production code should buffer the
                                 * remainder and register EPOLLOUT instead
                                 * of busy-retrying. */
                                continue;
                            }
                            perror("write");
                            closed = 1;
                            break;
                        }
                        sent += nw;
                    }
                    if (closed) break;
                }

                if (closed)
                    close_fd(epfd, fd);
            }
        }
    }

#ifdef KCUT_TCP
    kcut_cleanup();
#endif
      
    close(epfd);
    close(lfd);
    return 0;
}
