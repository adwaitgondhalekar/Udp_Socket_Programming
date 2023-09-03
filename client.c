#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#define PORT 5000                   // port number used by server socket
#define chunksize 32                //default chunk size

int client_socket, server_addr_length;
struct sockaddr_in server;
int base = 1, current_packet = 1, window_size, total_packets, i = 0, pack_left;
pthread_t thread;

//struct for data packets
struct data_packet
{
    int packet_no;
    int packet_size;
    int retransmitted;
    // char data[33];
    char data[chunksize + 1];
};

// struct for retransmitted data packets
struct ack_pending
{

    int packet_size;
    // char d[33];
    char d[chunksize + 1];
    int pack_no;
    struct ack_pending *next;
};

struct ack_pending *head = NULL;
struct ack_pending *curr = NULL;

// suubstring logic for slicing the buffer containing filedata into chunks
void substring(char *str, char *substr, int pos, int size)
{

    int k = 0;
    while (k < size)
    {
        *(substr + k) = str[k + pos];
        k++;
    }
    *(substr + size) = '\0';
    //printf("Substring - %s\n", substr);
}

// function triggered if ack not received for a packet after timeout
void timeout_handler(int signum)
{
    if (head != NULL)
    {
        struct data_packet lost_packet; // creating data_packet instance for lost packet
        lost_packet.packet_no = head->pack_no;
        lost_packet.packet_size = head->packet_size;
        strcpy(lost_packet.data, head->d);
        // filling dummy data to fill out remaining space in char array
        int strlength = strlen(lost_packet.data);
        if (strlength < chunksize)
        {
            for (int k = strlength; k < (chunksize + 1); k++)
            {
                lost_packet.data[k] = '|';
            }
        }
        lost_packet.data[chunksize] = '\0';
        lost_packet.retransmitted = 1; // setting the retransmitted status as 1 so that retransmitted packet isnt dropped

        int retransmit = sendto(client_socket, &lost_packet, sizeof(lost_packet), 0, (struct sockaddr *)&server, server_addr_length);
        if (retransmit < 0)
        {
            printf("Error in retransmitting packet - %d\n", lost_packet.packet_no);
        }
        //printf("Retransmitted packet -%d \\ data-%s\n", lost_packet.packet_no, lost_packet.data);
        printf("Retransmitted packet-%d\tBase-%d\n", lost_packet.packet_no, base);
        // alarm(0);
    }
    else
    {
        return;
    }
}

void *receive_ack(void *arg)
{

    while (pack_left > 0)
    {
        int recvd_packno = 0;
        signal(SIGALRM, timeout_handler);
        alarm(1); // starting timer for window to retransmit packet after timeout
        int rec_ack = recvfrom(client_socket, &recvd_packno, sizeof(recvd_packno), 0, (struct sockaddr *)&server, &server_addr_length);
        if (rec_ack < 0)
        {
            printf("Error in receiving ack from server\n");
        }
        else
        {
            pack_left--;
            struct ack_pending *ptr = head;
            struct ack_pending *prev = NULL;

            while (ptr != NULL)
            {
                if (ptr->pack_no == recvd_packno && ptr == head)
                {
                    if (ptr->next != NULL)                      //if ack received belongs to head node then change head and change base
                    {
                        // alarm(0);
                        head = head->next;
                        base = head->pack_no;
                        free(ptr);
                        break;
                    }

                    else
                    {
                        if (ptr->pack_no == total_packets)      //if ack received is for last packet and only head node is present
                        {
                            // alarm(0);
                            head = head->next;
                            free(ptr);
                            break;
                        }
                        else                                    //if ack received for a packet and only head node is present
                        {
                            // alarm(0);
                            base = ptr->pack_no + window_size;
                            head = head->next;
                            free(ptr);
                            break;
                        }
                    }
                }
                else if (ptr->pack_no == recvd_packno)          //if ack received for a packet where node is present in between
                {
                    // alarm(0);
                    prev->next = ptr->next;
                    free(ptr);
                    ptr = NULL;
                    prev = NULL;
                    break;
                }

                prev = ptr;
                ptr = ptr->next;
            }
            printf("Received ack for packet-%d\tBase-%d\n", recvd_packno, base);

            // ORIGINAL LOGIC
            /*if (head == NULL && current_packet > total_packets)
            {
                break;
                return NULL;
            }*/
            /*if (head == NULL && i > total_packets) // new logic
            {
                break;
            }*/
        }
    }
    return NULL;
}

int main()
{
    client_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); // created a client socket
    if (client_socket < 0)
    {
        printf("Error in creating client socket! \n");
        exit(0);
    }
    printf("Client socket created successfully! \n");

    printf("Enter the window size to be used\n");
    scanf("%d",&window_size);

    // defining server struct parameters

    // struct sockaddr_in server;

    server.sin_family = AF_INET;                     // denotes address family that the server uses
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // denotes the server's ip address
    server.sin_port = htons(PORT);

    server_addr_length = sizeof(server);

    int ws_sent = sendto(client_socket, &window_size, sizeof(window_size), 0, (struct sockaddr *)&server, server_addr_length);
    if (ws_sent < 0)
    {
        printf("Error in communicating window size to server! \n");
        exit(0);
    }
    printf("Window size communicated to server successfully! \n");

    char filename[10]; // storing name of the file
    // double packetsize = 32; // defining default packet size
    // int psize = (int)packetsize;
    double packetsize = (double)chunksize;              //creating double packetsize variable to count total packets further below
    // packetsize = psize;

    double filesize;
    printf("Enter the name of the file to be read\n");
    scanf("%s", filename);

    FILE *file = fopen(filename, "r");
    fseek(file, 0L, SEEK_END);                          // seeking to the end of file
    filesize = ftell(file);                             // finding current position of file ptr
    fseek(file, 0L, SEEK_SET);                          // resetting file pointer to initial position

    char *filedata = (char *)malloc(filesize);          // buffer for storing file contents
    fread(filedata, filesize, 1, file);                 // reading entire file contents into the buffer

    total_packets = (int)ceil((filesize) / packetsize); // calculating the total number of packets required to transfer the file
    pack_left = total_packets;
    int sent_tot_pack = sendto(client_socket, &total_packets, sizeof(total_packets), 0, (struct sockaddr *)&server, server_addr_length);
    if (sent_tot_pack < 0)
    {
        printf("Error in communicating total packets to server !\n");
        exit(0);
    }

    int local_base;
    int pos = 0;

    while (current_packet <= total_packets)
    {
        struct data_packet data_pack;
        local_base = base;
        // int run=0;

        // loop to send outstanding packets for the current window
        for (i = current_packet; i <= (base + window_size - 1); i++) // HERE I WAS INITIALLY LOCAL VARIABLE DECLARED IN THE FOR LOOP
        {
            if (i > total_packets)
                break;

            // for transmitting normal data packets
            else
            {
                // char cdata[psize + 1];
                char cdata[chunksize + 1];

                // fetch data for packet before sending

                // substring(filedata, cdata, pos, psize); // reading 32 bytes of data from file buffer sequentially
                substring(filedata, cdata, pos, chunksize);
                // printf("Packet %d - %s\n", i, cdata);   // printing chunk of data

                // creating a data packet and populating it with the data to be transferred
                data_pack.packet_no = i;


                // filling dummy data to fill out remaining space in char array
                int strlength = strlen(cdata);
                if (strlength < chunksize)
                {
                    for (int k = strlength; k < (chunksize + 1); k++)
                    {
                        cdata[k] = '|';
                    }
                }
                strcpy(data_pack.data, cdata);

                // data_pack.data[33] = '\0';
                data_pack.packet_size = sizeof(data_pack);
                data_pack.retransmitted = 0;


                // sending packet to server
                int send_pack = sendto(client_socket, &data_pack, sizeof(data_pack), 0, (struct sockaddr *)&server, server_addr_length);
                if (send_pack < 0)
                {
                    printf("Error in sending data packet-%d\n", data_pack.packet_no);
                    exit(0);
                }
                printf("Packet sent-%d \t Base - %d\n", data_pack.packet_no, base);
                //printf("Data in packet -%d \\ %s\n", data_pack.packet_no, data_pack.data);
                current_packet++;


                // creating pending ack node in linked list

                if (head == NULL)
                {
                    head = (struct ack_pending *)malloc(sizeof(struct ack_pending));
                    head->next = NULL;
                    head->pack_no = data_pack.packet_no;
                    head->packet_size = data_pack.packet_size;
                    strcpy(head->d, data_pack.data);
                    // head->d[32]='\0';
                    curr = head;
                }
                else
                {
                    struct ack_pending *new_node = (struct ack_pending *)malloc(sizeof(struct ack_pending));
                    new_node->next = NULL;
                    // new_node->ack_status = 0;
                    new_node->pack_no = data_pack.packet_no;
                    new_node->packet_size = data_pack.packet_size;
                    strcpy(new_node->d, data_pack.data);
                    // new_node->d[32]='\0';
                    curr->next = new_node;
                    curr = curr->next;
                }

                // pos = pos + psize;
                pos = pos + chunksize;
            }
        }

        // launching a thread to receive acks
        if (base == 1)
        {
            // thread to receive ack continuously
            int thread_err = pthread_create(&thread, NULL, receive_ack, NULL);
        }
        do
        {
            int dummy = 0;
        } while (local_base == base);
    }
    pthread_join(thread, NULL);
    fclose(file);
    //printf("Outside sending loop\n");


    // creating END packet
    struct data_packet end_pack;
    end_pack.packet_no = 0;
    strcpy(end_pack.data, "END");
    end_pack.packet_no = sizeof(end_pack);
    end_pack.retransmitted = 1;

    // sending END packet to server
    int end_packet = sendto(client_socket, &end_pack, sizeof(end_pack), 0, (struct sockaddr *)&server, server_addr_length);
    if (end_packet < 0)
    {
        printf("Error in sending data packet-%d\n", end_pack.packet_no);
        exit(0);
    }
    printf("End Packet sent-%d\n", end_pack.packet_no);

    close(client_socket);
    printf("File sent successfully \n");
}