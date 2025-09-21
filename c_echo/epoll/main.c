#define _GNU_SOURCE

#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <fcntl.h>

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

	// AF_INET: ip
	// SOCK_STREAM: TCP
	// 第3引数に関しては、第1引数と、第2引数で通信方法が一意に決まる場合、0でよい
	// man 7 socket 参照
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        fprintf(stderr, "failed to create a socket\n");
        exit(EXIT_FAILURE);
    }

	// 通信規格がipの場合、sockaddr_in を使用する
	// man 7 ip 参照
    struct sockaddr_in my_addr = {
		// 通信規格（IPV4, IPV6, etc.)
        .sin_family = AF_INET,
		// network byte order（ビックエンディアン）で保管するために、htons を使う
		// だいたいの CPU がリトルエンディアンを採用してるため、ビックエンディアンに変換してあげないと
		// 意図してない値になる
		// man 7 ip 参照
		// port 番号は 16 ビットのため、htons を使う
        .sin_port = htons(8124),
		// 任意の IP からのリクエストを受け付ける
		// man 7 ip 参照
        .sin_addr = {
            .s_addr = INADDR_ANY,
        },
    };

	// socket を作成したことで、通信規格は指定できたが、どこに通信するかの情報がないため、引数で渡して紐付ける
	// ソケットに名前をつけると呼ばれるらしい
	// man 2 bind 参照
    if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
        perror("failed to bind\n");
        exit(EXIT_FAILURE);
    }

	// sfd をパッシブソケット（accept で受け付ける用のソケット）にする
	// backlog(第2引数)には接続待ち状態のソケットの上限を指定する
	// man 2 listen 参照
    if (listen(sfd, 100) == -1) {
        fprintf(stderr, "failed to listen\n");
        exit(EXIT_FAILURE);
    }

	// man 3 epoll_event を参照
    struct epoll_event ev, events[MAX_EVENTS];
	int epoll_fd, nfds, i;

	// epoll_create の拡張版の epoll_create1 を使用したほうがいいが、課題で許可されてないため、epoll_create を使用する
	// epoll_create の引数は無視されるが、形式的に0より大きい値を渡す必要がある
	// man epoll_create 参照
	epoll_fd = epoll_create(1);
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

	// epoll の interest list を操作するために使用する
	// EPOLL_CTL_ADD で epoll インスタンスの interest list に fd を追加できる
	// 下記であれば、ev.events に EPOLLIN を指定しているため、sfd が読み囲み可能になればユーザーに通知すると言う意味になる
	// interest list = epoll が監視対象として覚えているリスト
	// man 2 epoll_ctl 参照
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
		fprintf(stderr, "failed to epoll_ctl\n");
		exit(EXIT_FAILURE);
	}

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_size = sizeof(peer_addr);

    while (1) {
		// イベントの発生を待つために epoll_wait を使用する
		// イベントが発生した fd は ready list を経由して、events に格納される
		// timeout に -1 を設定した場合、epoll_wait はイベントが発生するまで無期限にブロックする
		// 戻り値はI/Oが可能になった fd の個数
		// man 2 epoll_wait 参照
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			fprintf(stderr, "failed to epoll_wait\n");
			exit(EXIT_FAILURE);
		}

		// イベントがあったfd の数分ループを回す
		for (i = 0; i < nfds; i++) {
			// events には登録したときに渡した ev が入ってる
			// ここに来る前に sfd を入れたから、サーバーのイベントかクライアントのイベントかを判定できる
			sess = events[i].data.ptr;
			if (sess->fd == sfd) {
				// 待機列にあるはじめの fd を取りだす
				// accept4 を使用すると、fcntl を使用しなくても第4引数に non-blocking fd として受け取るように指定できる
				// 今回は許可されてないため、使えない
				// man 2 accept 参照
				cfd = accept(sfd, (struct sockaddr*) &peer_addr, &peer_addr_size);
				if (cfd == -1) {
					fprintf(stderr, "failed to accept");
					exit(EXIT_FAILURE);
				}
				// fd のstate を取得する
				int flags = fcntl(cfd, F_GETFL, 0);
				if (flags == -1) {
					fprintf(stderr, "failed to fcntl\n");
					exit(EXIT_FAILURE);
				}
				// 前で取得した fd の state に NONBLOCK を追加する
				if (fcntl(cfd, F_SETFL, flags | O_NONBLOCK) == -1) {
					fprintf(stderr, "failed to fcntl\n");
					exit(EXIT_FAILURE);
				}
				printf("accepted\n");
				sess = (struct session *)malloc(sizeof(struct session));
				if (sess == NULL) {
					fprintf(stderr, "failed to malloc\n");
					exit(EXIT_FAILURE);
				}

				// レスポンスを返すためにクライアントの fd をEPOLLIN(読み込み可能かどうか)で監視対象に入れる
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
						// 読み込めるデータがなかった
						if (sess->len == 0) {
							// cfd を監視対象から外す
							if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sess->fd, NULL) == -1) {
								fprintf(stderr, "failed to epoll_ctl\n");
								exit(EXIT_FAILURE);
							}
							free(sess);
							close(sess->fd);
						} else {
							// 読み込めるデータがあった
							// 書き込みフラグをたてる
							sess->written = 0;
							sess->state = 1;
							// 書き込み可能かを監視下におく
							ev.events = EPOLLOUT;
							ev.data.ptr = (void *)sess;
							// cfd の状態を変更するため、MOD を使用する
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
						// 全部書ききった
						if (sess->written >= sess->len) {
							sess->state = 0;
							sess->len = 0;
							sess->written = 0;
							ev.events = EPOLLIN;
							ev.data.ptr = (void *)sess;
							// 読み込みとして再度登録する
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
