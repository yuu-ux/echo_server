#define _GNU_SOURCE

#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

#define MAX_EVENTS 10
#define BUF_SIZE 10

struct session {
    int fd;
    int state;
    ssize_t len;
    ssize_t written;
    uint8_t buf[1024];
};

int main(void) {
    int sfd, cfd;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        fprintf(stderr, "failed to create a socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in my_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8124),
        .sin_addr = {
            .s_addr = 0,
        },
    };

    if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
        perror("failed to bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, 100) == -1) {
        fprintf(stderr, "failed to listen\n");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[MAX_EVENTS];
	int epoll_fd, nfds, i;

	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		fprintf(stderr, "failed to epoll_create1\n");
		exit(EXIT_FAILURE);
	}

	ev.events = EPOLLIN;
    struct session *sess;
    sess = (struct session *)malloc(sizeof(struct session));
	if (sess == NULL) {
		fprintf(stderr, "failed to malloc\n");
		exit(EXIT_FAILURE);
	}
    sess->fd = sfd;
	ev.data.ptr = (void*)sess;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
		fprintf(stderr, "failed to epoll_ctl\n");
		exit(EXIT_FAILURE);
	}

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_size = sizeof(peer_addr);

    while (1) {
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			fprintf(stderr, "failed to epoll_wait\n");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < nfds; i++) {
			sess = events[i].data.ptr;
			if (sess->fd == sfd) {
				cfd = accept4(sfd, (struct sockaddr*) &peer_addr, &peer_addr_size, SOCK_NONBLOCK);
				if (cfd == -1) {
					fprintf(stderr, "failed to accept");
					exit(EXIT_FAILURE);
				}
				printf("accepted\n");
				sess = (struct session *)malloc(sizeof(struct session));
				if (sess == NULL) {
					fprintf(stderr, "failed to malloc\n");
					exit(EXIT_FAILURE);
				}

				sess->fd = cfd;
				sess->state = 0;
				sess->len = 0;
				sess->written = 0;
                ev.events = EPOLLIN;
				ev.data.ptr = (void *)sess;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
                    fprintf(stderr, "failed to epoll_ctl on cfd\n");
                    exit(EXIT_FAILURE);
                }
			} else {
                // peer socket
                switch (sess->state) {
                    case 0: // READ
						sess->len = read(sess->fd, sess->buf, 1024);
						if (sess->len == 0) {
							if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sess->fd, NULL) == -1) {
								fprintf(stderr, "failed to epoll_ctl\n");
								exit(EXIT_FAILURE);
							}
							free((void *)sess);
						} else {
							sess->written = 0;
							sess->state = 1;
							ev.events = EPOLLOUT;
							ev.data.ptr = (void *)sess;
							if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sess->fd, &ev) == -1) {
								fprintf(stderr, "failed to epoll_ctl\n");
								exit(EXIT_FAILURE);
							}
						}
                        break;
                    case 1: // WRITE
						;
						ssize_t ret;
						ret = write(sess->fd, sess->buf + sess->written, sess->len - sess->written);
						if (ret == -1) {
							fprintf(stderr, "failed to write\n");
							exit(EXIT_FAILURE);
						}
						sess->written += ret;
						if (sess->written >= sess->len) {
							sess->state = 0;
							sess->len = 0;
							sess->written = 0;
							ev.events = EPOLLIN;
							ev.data.ptr = (void *)sess;
							if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sess->fd, &ev) == -1) {
								fprintf(stderr, "failed to epoll_ctl\n");
								exit(EXIT_FAILURE);
							}
						}
                        break ;
                }
			}
		}
    }
    return 0;
}
