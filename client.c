#include <unistd.h> 
#include <stdio.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <stdlib.h> 
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/time.h>
#include <netinet/in.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define TIMEOUT 20      // in seconds
#define BUFFER_SIZE 128

int flag = 0;
void handle_alarm(int signal)
{
    flag = 1;
}

void generate_map(char** map, int WIDTH)
{
	for (int i = 0; i < 2 * WIDTH + 1; i++)
	{
		for (int j = 0; j < 2 * WIDTH + 2; j++)
		{
			if (i % 2 == 0 && j % 2 == 0)
				map[i][j] = '*';
			else
				map[i][j] = ' ';
		}
        map[i][2 * WIDTH + 1] = '\n';
	}
}

void print_map(char** map, int WIDTH)
{
    for (int i = 0; i < 2 * WIDTH + 1; ++i)
        write(STDOUT, map[i], 2 * WIDTH + 2);
}

int check_new_square(char** map, int putter, int WIDTH)
{
    int rv = -1;
    for (int i = 1; i < 2 * WIDTH + 1; i+=2)
    {
        for (int j = 1; j < 2 * WIDTH + 2; j+=2)
        {
            if (map[i - 1][j] != ' ' && map[i][j - 1] != ' ' && map[i + 1][j] != ' ' && map[i][j + 1] != ' ' && map[i][j] == ' ')
            {
                map[i][j] = putter + '0';
                rv = 0;
            }
        }
    }
    return rv;
}

int who_is_winner(char** map, int WIDTH)
{
    int ones = 0, twos = 0, threes = 0, fours = 0;
    for (int i = 1; i < 2 * WIDTH + 1; i+=2)
    {
        for (int j = 1; j < 2 * WIDTH + 1; j+=2)
        {
            if (map[i][j] == '1')
                ones += 1;
            else if(map[i][j] == '2')
                twos += 1;
            else if(map[i][j] == '3')
                threes += 1;
            else if(map[i][j] == '4')
                fours += 1;
            else
                return -1;
        }
    }
    int max = ones;
    if (max < twos)
        max = twos;
    if (max < threes)
        max = threes;
    if (max < fours)
        max = fours;
    if (max == ones)
        return 1;
    if (max == twos)
        return 2;
    if (max == threes)
        return 3;
    if (max == fours)
        return 4;
}

int main(int argc, char* argv[])
{
    struct sockaddr_in server_address; 
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) 
    { 
        write(STDERR, "ERROR: Creating client socket failed, exiting...\n", 50);
        exit(EXIT_FAILURE);
    }
    write(STDOUT, "Creating socket successful\n", 28);
   
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = inet_addr("‫‪127.0.0.1‬‬");
    server_address.sin_port = htons(atoi(argv[1])); 
       
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)  
    { 
        write(STDERR, "ERROR: Address not support, exiting...\n", 40);
        exit(EXIT_FAILURE);
    }
    write(STDOUT, "Converting address from text to binary successful\n", 51);

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) 
    { 
        write(STDERR, "ERROR: Connection Failed, exiting...\n", 38);
        exit(EXIT_FAILURE);
    }
    write(STDOUT, "Connection successful\n", 23);
    
    // send game mode
    char buffer[BUFFER_SIZE] = {0};
    char number_of_players[BUFFER_SIZE] = {0};
    read(client_socket , buffer, BUFFER_SIZE); 
    write(STDOUT, buffer, BUFFER_SIZE);
    write(STDOUT, "\nChoose: ", 10);
    int number_of_data = read(STDIN, number_of_players, BUFFER_SIZE);
    write(client_socket, number_of_players, number_of_data);
    number_of_data = read(client_socket, buffer, 43);
    write(STDOUT, buffer, number_of_data);

    // wait for other players
    char game_port[7] = {0};
    recv(client_socket, game_port, 7, 0);
    close(client_socket);

    // num_of_players -> how many players we have
    // port -> port of the game
    int turn = atoi(game_port) % 10;
    int port = atoi(game_port) / 10;
    int num_of_players = atoi(number_of_players);
    struct sockaddr_in binded, not_binded; 
    int game_socket = socket(AF_INET, SOCK_DGRAM, 0);
    int opt1 = 1, opt2 = 1;

    if (game_socket < 0)
    { 
        write(STDERR, "ERROR: Creating game socket failed, exiting...\n", 48);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(game_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&opt1, sizeof(opt1)) < 0 ||
        setsockopt(game_socket, SOL_SOCKET, SO_BROADCAST, (char*)&opt2, sizeof(opt2)) < 0)
    {
        write(STDERR, "ERROR: 'setsockopt' failed, exiting...\n", 40);
        exit(EXIT_FAILURE);
    }
    
    binded.sin_family = AF_INET; 
    binded.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
    binded.sin_port = htons(port);

    not_binded.sin_family = AF_INET; 
    not_binded.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
    not_binded.sin_port = htons(port);

    if (bind(game_socket, (struct sockaddr*)&binded, sizeof(binded)) < 0)
    { 
        write(STDERR, "ERROR: Binding failed, exiting...\n", 35); 
        exit(EXIT_FAILURE);
    }

    write(STDOUT, "GAME: Game started!\n", 21);
    write(STDOUT, "GAME: You are player number ", 29);
    char* turn_c = (char*)malloc(2);
    turn_c[0] = (turn + 1) + '0';
    turn_c[1] = '\n';
    write(STDOUT, turn_c, 2);
    int id = turn + 1;

    int WIDTH = num_of_players + 1;

    signal(SIGALRM, handle_alarm);

    char** map = (char**)malloc((2 * WIDTH + 1) * sizeof(char**));
    for (int i = 0; i < 2 * WIDTH + 1; i++)
        map[i] = (char*)malloc((2 * WIDTH + 2) * sizeof(char*));
    generate_map(map, WIDTH);
    print_map(map, WIDTH);

    while (1)
    {
        alarm(TIMEOUT);
        if (turn == 0)
        {
            if (who_is_winner(map, WIDTH) != -1)
                break;
            char* coor = (char*)malloc(5);
            write(STDOUT, "\nGAME: It's your turn now\nEnter x,y: ", 38);
            
            fd_set reads;
            FD_ZERO(&reads);
            FD_SET(STDIN, &reads);
            struct timeval timeout;
            timeout.tv_sec = TIMEOUT;
            timeout.tv_usec = 0;
            if (select(STDIN + 1, &reads, NULL, NULL, &timeout) > 0)
            {
                read(STDIN, coor, 5);
                coor[4] = turn_c[0];
                if (coor[1] == ',')
                    coor[3] = 'x';
                sendto(game_socket, coor, 6, 0, (struct sockaddr*)&binded, sizeof(binded));
                int add_len = sizeof(not_binded);
                // fake receive to disable loopback.
                recvfrom(game_socket, coor, 6, 0, (struct sockaddr*)&not_binded, &add_len);

                int x = coor[0] - '0';
                int y = coor[2] - '0';

                if (coor[1] != ',')
                {
                    x = 10;
                    y = coor[3] - '0';
                }
                if (x % 2 == 1 && y == 1 && num_of_players == 4)
                {
                    map[x][10] = '|';
                }
                else
                {
                    if (x % 2 == 1)
                        map[x][y] = '|';
                    else
                        map[x][y] = '-';
                }
                alarm(TIMEOUT);
            }
            int res = check_new_square(map, id, WIDTH);                
            if (res == -1)
                turn = num_of_players - 1;
            free(coor);
            if (flag == 1)
            {
                write(STDOUT, "\nGAME: Your time is up!\n", 25);
                flag = 0;
            }
            print_map(map, WIDTH);
        }
        else
        {
            if (who_is_winner(map, WIDTH) != -1)
                break;
            write(STDOUT, "\nGAME: Waiting for other players to move. \n", 44);
            char coor[6];
            int add_len = sizeof(not_binded);

            fd_set reads;
            FD_ZERO(&reads);
            FD_SET(game_socket, &reads);
            struct timeval timeout;
            timeout.tv_sec = TIMEOUT;
            timeout.tv_usec = 0;
            int res = -1;
            if (select(game_socket + 1, &reads, NULL, NULL, &timeout) > 0)
            {
                recvfrom(game_socket, coor, 6, 0,(struct sockaddr*)&not_binded, &add_len);
                int x = coor[0] - '0';
                int y = coor[2] - '0';
                int sender = coor[4] - '0';
                char sender_c[2] = {0};
                sender_c[0] = coor[4];

                if (coor[1] != ',')
                {
                    x = 10;
                    y = coor[3] - '0';
                }

                coor[3] = '\n';
                coor[4] = '\0';
                write(STDOUT, "GAME: Player ", 14);
                write(STDOUT, sender_c, 1);
                write(STDOUT, " picked ", 9);
                write(STDOUT, coor, 5);
                

                if (x % 2 == 1 && y == 1 && num_of_players == 4)
                {
                    map[x][10] = '|';
                }
                else
                {
                    if (x % 2 == 1)
                        map[x][y] = '|';
                    else
                        map[x][y] = '-';
                }
                res = check_new_square(map, sender, WIDTH);
            }

            print_map(map, WIDTH);
            if (res == -1)
                turn -= 1;
            if (flag == 1)
                flag = 0;
        }
    }
    if (who_is_winner(map, WIDTH) == id)
        write(STDOUT, "GAME: You won!\n", 16);
    else
        write(STDOUT, "GAME: You lost :(\n", 19);

    free(map);
    return 0;
}