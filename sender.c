#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
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

// timeout duration (in seconds) for retransmission (RTO)
#define TIMEOUT 2

#define BUFSIZE 1024 // max size (in bytes) for data in a message
#define N 2          // modulo used for numbering messages

// message format
struct message {
    uint8_t seq;        // sequence number of the message
    char data[BUFSIZE]; // data contained in the message
};

noreturn void usage(const char *msg)
{
    fprintf(stderr, "usage: %s local_port dest_ip dest_port\n", msg);
    exit(EXIT_FAILURE);
}

void send_loop(int socket)
{
    ssize_t n;
    struct message m;

    // we start by sending message 0 (see line 56)
    m.seq = 1;

    while ((n = read(STDIN_FILENO, m.data, BUFSIZE)) > 0) {
        // send the data
        m.seq = (m.seq + 1) % 2;
        CHK(send(socket, &m, 1 + n, 0));

        // wait (blocking) for ACK from receiver
        ssize_t r;
        struct message tmp;

        // the condition on the right of OR is useful if the line is full-duplex 
        while (((r = recv(socket, &tmp, sizeof tmp, 0)) == -1 && errno == EAGAIN) || tmp.seq != m.seq)
            // retransmission!
            CHK(send(socket, &m, 1 + n, 0));
        CHK(r);
    }
    CHK(n);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        usage(argv[0]);

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

    // set timeout for socket when no data is received
    struct timeval tv = {TIMEOUT, 0};
    CHK(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv));

    // receiver parameters (medium in this case)
    struct addrinfo *dest;
    hints.ai_family = AF_UNSPEC; // argv[2] can be either IPv4 or IPv6 address
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    CHKA(getaddrinfo(argv[2], argv[3], &hints, &dest));

    // lock the socket for exclusive communication with the medium
    // all other sources or destinations will be filtered by the OS
    CHK(connect(sockfd, dest->ai_addr, dest->ai_addrlen));
    freeaddrinfo(dest);

    send_loop(sockfd);

    // close socket
    CHK(close(sockfd));

    return 0;
}
