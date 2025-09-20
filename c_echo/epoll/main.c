#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

#define MAX_EVENTS 10

int main(void) {
    int sfd, cfd;

    printf("hello\n");
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
        fprintf(stderr, "failed to bind\n");
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
	ev.data.fd = sfd;
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

		for (i =0; i < nfds; i++) {
			if (events[i].data.fd == sfd) {
				cfd = accept(sfd, (struct sockaddr*) &peer_addr, &peer_addr_size);
				if (cfd == -1) {
					fprintf(stderr, "failed to accept");
					exit(EXIT_FAILURE);
				}
				printf("accepted\n");

			} else {

			}
		}

        ssize_t len;
        uint8_t buf[1024];
        while ((len = read(cfd, buf, 1024)) != 0) {
            ssize_t written = 0;
            while (written < len) {
                ssize_t ret = write(cfd, buf+written, len-written);
                if (ret == -1) {
                    fprintf(stderr, "failed to write\n");
                    exit(EXIT_FAILURE);
                }
                written += ret;
            }
        }
        printf("closed\n");
    }
    printf("%d ok\n", cfd);
    return 0;
}
