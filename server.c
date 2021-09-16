#include <unistd.h> 
#include <stdio.h> 
#include <fcntl.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define BUFFER_SIZE 1024
#define NUMBER_OF_PENDING_REQUESTS 100

struct TwoPlayerGame
{
    int n;
    int fds[2];
} twopg;

struct ThreePlayerGame
{
    int n;
    int fds[3];
} threepg;

struct FourPlayerGame
{
    int n;
    int fds[4];
} fourpg;


char* to_string(char* res, int num)
{
    int i = 1, j = BUFFER_SIZE;
    while (num / i != 0)
    {
        res[j] = (num % (i * 10)) / i + '0';
        i = i * 10;
        j = j - 1;
    }
    return res + BUFFER_SIZE - j + 1;
}

int main(int argc, char* argv[])
{
    int BASE_PORT = 12345;
    twopg.n = 0;
    threepg.n = 0;
    fourpg.n = 0;
    int server_descriptor;
    struct sockaddr_in address;
    // IPV4, TCP, protocol = 0
    server_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_descriptor == 0)
    {
        write(STDERR, "ERROR: Creating server socket failed, exiting...\n", 50);
        exit(EXIT_FAILURE);
    }
    write(STDOUT, "SERVER: Creating server socket successful\n", 43);

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1])); 

    if (bind(server_descriptor, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        write(STDERR, "ERROR: Binding failed, exiting...\n", 35);
        exit(EXIT_FAILURE);
    }
    write(STDOUT, "SERVER: Binding successful\n", 28);

    if (listen(server_descriptor, NUMBER_OF_PENDING_REQUESTS) < 0)
    {
        write(STDERR, "ERROR: listening failed, exiting...\n", 37);
        exit(EXIT_FAILURE);
    }
    write(STDOUT, "SERVER: Now server is ready and listening to the port...\n", 58);
    
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server_descriptor, &reads);
    
    struct timeval timeout;
    timeout.tv_sec = 10 * 60; // 10 minutes
    timeout.tv_usec = 0;

    char buffer[1024] = {0};
    int max_fd = server_descriptor;
    int new_fd = 0;
    while (1)
    {
        fd_set reads_copy;
        memcpy(&reads_copy, &reads, sizeof(reads));
        int number_of_ready_fds = select(max_fd + 1, &reads_copy, NULL, NULL, &timeout);
        for (int i = 0; i <= max_fd && number_of_ready_fds > 0; ++i)
        {
            if (FD_ISSET(i, &reads_copy) == 1)
            {
                number_of_ready_fds -= 1;
                // if there is a new client
                if (i == server_descriptor)
                {
                    new_fd = accept(server_descriptor, NULL, NULL);
                    FD_SET(new_fd, &reads);
                    if (new_fd > max_fd)
                        max_fd = new_fd;

                    char* res = (char*)malloc(BUFFER_SIZE);
                    char* fd_in_str = to_string(res, new_fd);                        
                    write(STDOUT, "SERVER: Client with socket descriptor ", 39);
                    write(STDOUT, fd_in_str, BUFFER_SIZE);
                    write(STDOUT, " has arrived.\n", 15);
                    free(res);
                    write(new_fd, "SERVER: Please choose game mode: (2: 2 player, 3: 3 player, 4: 4 Player)", 73);
                }
                else
                {
                    recv(i, buffer, BUFFER_SIZE, 0);
                    switch (atoi(buffer))
                    {
                        case 2:
                            twopg.fds[twopg.n] = i;    
                            twopg.n += 1;
                            break;
                        case 3:
                            threepg.fds[threepg.n] = i;
                            threepg.n += 1;
                            break;
                        case 4:
                            fourpg.fds[fourpg.n] = i;
                            fourpg.n += 1;
                            break;    
                        default:
                            break;
                    }
                    char* res = (char*)malloc(BUFFER_SIZE);
                    char* fd_in_str = to_string(res, i);                        
                    write(STDOUT, "SERVER: Client with socket descriptor ", 39);
                    write(STDOUT, fd_in_str, BUFFER_SIZE);
                    write(STDOUT, " chose game mode: ", 19);
                    write(STDOUT, buffer, BUFFER_SIZE);
                    free(res);
                    write(i, "SERVER: Please wait for other players...\n\0", 43);
                    memset(buffer, 0, BUFFER_SIZE);
                }
                if (twopg.n == 2)
                {
                    for (int i = 0; i < twopg.n; ++i)
                    {
                        // We assume port has 5 digit.
                        char port[7];
                        memset(port, 0, 7);
                        int x = 1, y = 4;
                        while (BASE_PORT / x != 0)
                        {
                            port[y] = (BASE_PORT % (x * 10)) / x + '0';
                            x = x * 10;
                            y = y - 1;
                        }
                        port[6] = '\0';
                        port[5] = i + '0';
                        send(twopg.fds[i], port, 7, 0);
                        FD_CLR(twopg.fds[i], &reads);
                        if (twopg.fds[i] == max_fd)
                            while (FD_ISSET(max_fd, &reads) == 0)
                                max_fd -= 1;
                        close(twopg.fds[i]);
                    }
                    BASE_PORT += 1;
                    twopg.n = 0;
                    write(STDOUT, "SERVER: A two player game was filled.\n", 39);
                }
                if (threepg.n == 3)
                {
                    for (int i = 0; i < threepg.n; ++i)
                    {
                        char port[7];
                        memset(port, 0, 7);
                        int x = 1, y = 4;
                        while (BASE_PORT / x != 0)
                        {
                            port[y] = (BASE_PORT % (x * 10)) / x + '0';
                            x = x * 10;
                            y = y - 1;
                        }
                        port[6] = '\0';
                        port[5] = i + '0';
                        send(threepg.fds[i], port, 7, 0);
                        FD_CLR(threepg.fds[i], &reads);
                        if (threepg.fds[i] == max_fd)
                            while (FD_ISSET(max_fd, &reads) == 0)
                                max_fd -= 1;
                        close(threepg.fds[i]);
                    }
                    BASE_PORT += 1;
                    threepg.n = 0;
                    write(STDOUT, "SERVER: A three player game was filled.\n", 41);
                }
                if (fourpg.n == 4)
                {
                    for (int i = 0; i < fourpg.n; ++i)
                    {
                        char port[7];
                        memset(port, 0, 7);
                        int x = 1, y = 4;
                        while (BASE_PORT / x != 0)
                        {
                            port[y] = (BASE_PORT % (x * 10)) / x + '0';
                            x = x * 10;
                            y = y - 1;
                        }
                        port[6] = '\0';
                        port[5] = i + '0';
                        send(fourpg.fds[i], port, 7, 0);
                        FD_CLR(fourpg.fds[i], &reads);
                        if (fourpg.fds[i] == max_fd)
                            while (FD_ISSET(max_fd, &reads) == 0)
                                max_fd -= 1;
                        close(fourpg.fds[i]);
                    }
                    BASE_PORT += 1;
                    fourpg.n = 0;
                    write(STDOUT, "SERVER: A four player game was filled.\n", 40);
                }
            }
        }
    }
    close(server_descriptor);
    return 0;
}