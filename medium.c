#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <poll.h>
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

#define BUFSIZE 2048

noreturn void usage(const char *msg)
{
    fprintf(stderr,
            "usage: %s local_port sender_ip sender_port receiver_ip "
            "receiver_port loss_rate\n",
            msg);
    exit(EXIT_FAILURE);
}

void perte(double error, int sockfd, char *buffer, ssize_t n)
{
    double r = (double)rand() / RAND_MAX;
    if (r > error) {
        CHK(send(sockfd, buffer, n, 0));
        printf("transmitted\n");
    } else {
        printf("lost\n");
    }
    fflush(stdout);
}

void main_loop(double error, int sock_send, int sock_recv)
{
    // two incoming sockets: sender and receiver
    struct pollfd pfds[2];
    pfds[0].fd = sock_send;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock_recv;
    pfds[1].events = POLLIN;

    ssize_t n;
    char buffer[BUFSIZE];

    while (1) {
        // waiting for data on either socket
        CHK(poll(pfds, 2, -1));

        // data from sender
        if (pfds[0].revents & POLLIN) {
            CHK(n = recv(sock_send, buffer, BUFSIZE, 0));
            if (n > 0) {
                printf("S->R, seq=%d -> ", buffer[0]);
                perte(error, sock_recv, buffer, n);
            }
        }

        // data from receiver (ACK/control)
        if (pfds[1].revents & POLLIN) {
            CHK(n = recv(sock_recv, buffer, BUFSIZE, 0));
            printf("R->S, seq=%d -> ", buffer[0]);
            perte(error, sock_send, buffer, n);
        }
    }
}

int socket_factory(char *local_port, char *ip, char *port)
{
    int sockfd;

    // local address structure
    if (local_port != NULL) {
        struct addrinfo *local, hints = {0};
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
        CHKA(getaddrinfo(NULL, local_port, &hints, &local));

        // create a dual-stack socket (IPv4 and IPv6)
        int value = 0;
        CHK(sockfd = socket(local->ai_family, local->ai_socktype, local->ai_protocol));
        CHK(setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof value));

        // bind socket to address/port
        CHK(bind(sockfd, local->ai_addr, local->ai_addrlen));
        freeaddrinfo(local);
    }

    // remote address structure
    struct addrinfo *dest, hints2 = {0};
    hints2.ai_family = AF_UNSPEC;
    hints2.ai_socktype = SOCK_DGRAM;
    hints2.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    CHKA(getaddrinfo(ip, port, &hints2, &dest));

    if (local_port == NULL) 
        CHK(sockfd = socket(dest->ai_family, dest->ai_socktype, dest->ai_protocol));

    // restrict the socket to exclusive communication
    CHK(connect(sockfd, dest->ai_addr, dest->ai_addrlen));
    freeaddrinfo(dest);

    return sockfd;
}

void quit(int signo)
{
    (void)signo;
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 7)
        usage(argv[0]);

    // initialize pseudo-random number generator
    srand(time(NULL));

    // cleanly exit the program if SIGTERM is received
    struct sigaction sa;
    sa.sa_handler = quit;
    sa.sa_flags = 0;
    CHK(sigemptyset(&sa.sa_mask));
    CHK(sigaction(SIGTERM, &sa, NULL));

    // create the sockets
    int sock_send, sock_recv;
    sock_send = socket_factory(argv[1], argv[2], argv[3]);
    sock_recv = socket_factory(NULL, argv[4], argv[5]);

    // packet loss rate
    double erreur;
    if (sscanf(argv[6], "%lf", &erreur) != 1 || erreur > 1.0 || erreur < 0) {
        fprintf(stderr, "usage: error rate must be between [0,1]\n");
        exit(EXIT_FAILURE);
    }

    // wait for messages
    main_loop(erreur, sock_send, sock_recv);

    // close sockets
    CHK(close(sock_send));
    CHK(close(sock_recv));

    return 0;
}
