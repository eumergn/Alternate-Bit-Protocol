#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define CHK(op)                                                                \
    do {                                                                       \
        if ((op) == -1) {                                                      \
            perror(#op);                                                       \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define CHKA(op)                                                               \
    do {                                                                       \
        int error = 0;                                                         \
        if ((error = (op)) != 0) {                                             \
            fprintf(stderr, #op " %s\n", gai_strerror(error));                 \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

#define BUFSIZE 1024 // max size (in bytes) for data in a message
#define N 2          // modulo used to number the messages

// message format
struct message {
    uint8_t seq;        // message sequence number
    char data[BUFSIZE]; // data contained in the message
};

noreturn void usage(const char *msg)
{
    fprintf(stderr, "usage: %s local_port\n", msg);
    exit(EXIT_FAILURE);
}

void recv_loop(int socket)
{
    ssize_t n;
    uint8_t ma = 0; // expected message
    struct message m;

    // sender can use an IPv4 or IPv6 address
    // struct sockaddr_storage is large enough to store any type of address
    struct sockaddr_storage ss;
    struct sockaddr *s = (struct sockaddr *)&ss;
    socklen_t addrlen = sizeof ss;

    while ((n = recvfrom(socket, &m, sizeof m, 0, s, &addrlen)) > 0) {
        if (m.seq == ma) { // received the expected message
            CHK(write(STDOUT_FILENO, m.data, n - 1));
            ma = (ma + 1) % N;
        }

        // only send the sequence number of the last correctly received message
        CHK(sendto(socket, &m, 1, 0, s, addrlen));
    }
    CHK(n);
}

void quit(int signo)
{
    (void)signo;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        usage(argv[0]);

    // exitting w SIGTERM
    struct sigaction sa;
    sa.sa_handler = quit;
    sa.sa_flags = 0;
    CHK(sigemptyset(&sa.sa_mask));
    CHK(sigaction(SIGTERM, &sa, NULL));

    // create an IPv6 socket that accepts IPv4 communications
    int sockfd, value = 0;
    CHK(sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP));
    CHK(setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof value));

    // address structure with local parameters
    // (all IPv6 addresses of the host + port passed as argument)
    // getaddrinfo here does a simple conversion
    // (no DNS queries or file lookups)
    struct addrinfo *local, hints = {0};
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    CHKA(getaddrinfo(NULL, argv[1], &hints, &local));

    // bind socket <-> address/port
    CHK(bind(sockfd, local->ai_addr, local->ai_addrlen));
    freeaddrinfo(local);

    // receive using alternating-bit protocol
    recv_loop(sockfd);

    // close socket
    CHK(close(sockfd));

    return 0;
}
