#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#define PORT 5000           // denotes the port number used by server socket
#define chunksize 32        // default chunk size

int total_packets, received_packets = 0;

struct data_packet
{
    int packet_no;
    int packet_size;
    int retransmitted;
    // char data[33];
    char data[chunksize + 1];
};

int main()
{

    int server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); // created server socket
    if (server_socket < 0)
    {
        printf("Error in creating server socket! \n");
        exit(0);
    }
    printf("Server socket created successfully! \n");

    // defining server struct parameters

    struct sockaddr_in server, client;

    server.sin_family = AF_INET;                // denotes the address family used by server socket
    server.sin_addr.s_addr = htonl(INADDR_ANY); // denotes ip address used by server socket
    server.sin_port = htons(PORT);              // denotes the port used by server socket

    int client_addr_length = sizeof(client);
    // binding the server socket to port number

    int bind_server = bind(server_socket, (struct sockaddr *)&server, sizeof(server));

    if (bind_server < 0)
    {
        printf("Error in binding socket to port and local addr! \n");
        exit(0);
    }
    printf("Server socket bound to port and local addr successfully! \n");

    // receiving window size from server

    int window_size;

    int recv_ws = recvfrom(server_socket, &window_size, sizeof(window_size), 0, (struct sockaddr *)&client, &client_addr_length);

    if (recv_ws < 0)
    {
        printf("Error in receiving window size from client! \n");
        exit(0);
    }
    printf("Received window size - \"%d\" from client successfully! \n", window_size);
    int recv_tot_pack = recvfrom(server_socket, (int *)&total_packets, sizeof(total_packets), 0, (struct sockaddr *)&client, &client_addr_length);
    if (recv_tot_pack < 0)
    {
        printf("Error in receiving total packet count!\n");
        exit(0);
    }

    // receiving the number of total packets to be received

    printf("Total packets to be received from client - %d packets\n", total_packets);

    char received_data[total_packets][chunksize + 1];

    FILE *file;
    file = fopen("out.txt", "w+");
    time_t t;

    /* Intializes random number generator */
    srand48(time(NULL));
    // srand((unsigned)time(&t));
    // srand(8);

    // receiving data packets one by one
    while (1)
    {

        struct data_packet pack;
        pack.packet_no = 0;
        pack.packet_size = 0;
        pack.retransmitted = 0;
        pack.data[chunksize] = '\0';

        int recv_pack = recvfrom(server_socket, &pack, sizeof(struct data_packet), 0, (struct sockaddr *)&client, &client_addr_length);
        if (recv_pack < 0)
        {
            printf("Error in receiving data packet\n");
            exit(0);
        }
        else
        {

            if (strcmp("END", pack.data) == 0)
            {
                // printf("Data - %s\t Packet no - %d\n", pack.data, pack.packet_no);
                printf("Received END packet\n");
                break;
            }

            // simulating packet drop by random number generation

            // int accept_packet = (rand() % 3); // if accept_packet==0 then drop packet

            // int accept_packet=1;
            //  int pack_drop = 1;
            //  if (pack.retransmitted == 1 || pack.data == "END")
            //      accept_packet = 1;

            if (drand48() > 0.3 || pack.retransmitted == 1 || pack.data == "END") // packet not dropped
            {
                // 30% drop rate

                // printf("Data - %s\t Packet no - %d\n", pack.data, pack.packet_no); // printing data from the received packet

                printf("Received packet no -%d\n", pack.packet_no);
                // storing received strings into an array
                strcpy(received_data[(pack.packet_no - 1)], pack.data);
                received_data[(pack.packet_no - 1)][chunksize] = '\0';

                // sending acknowledgements of received data packets
                // usleep(800000);
                for (int del = 0; del < 10000; del++)
                {
                }


                int ack_status = sendto(server_socket, (void *)&pack.packet_no, sizeof(pack.packet_no), 0, (struct sockaddr *)&client, client_addr_length);
                if (ack_status < 0)
                {
                    printf("Error in sending ack for packet - %d\n", pack.packet_no);
                    exit(0);
                }
                printf("Ack sent for packet - %d\n", pack.packet_no);
            }
            else
            {
                printf("Packet - %d  dropped\n", pack.packet_no);
            }

            // fclose(file);
        }
    }

    // fclose(file);
    printf("File reached successfully\n");

    // writing the contents received from client to a file
    file = fopen("out.txt", "w");
    for (int k = 0; k < total_packets; k++)
    {
        // fprintf(file, "%s", received_data[k]);
        for (int j = 0; j < (chunksize); j++)
        {
            if (received_data[k][j] != '|')
                fputc(received_data[k][j], file);
        }
    }

    printf("Data written to out.txt successfully\n");

    fclose(file);

    /*for (int k = 0; k < total_packets; k++)
    {

        printf("%s",received_data[k]);
    }*/
    close(server_socket);
}