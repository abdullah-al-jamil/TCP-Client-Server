#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include "event_loop.h"
#define MAX_LEN 500
#define TIMEOUT 5
#define EXTRA_BYTES 4 //version=2 , options=2 , event=2
#define log_msg(type, ...) printf(__VA_ARGS__)
#define PORT 8080
#define host_no "127.0.0.1"
#define macaddress "macaddress"

static void sigpipe_handler(int signum) {}

information inform;

typedef enum
{
    NOT_INITIALIZED,
    NOT_CONNECTED,
    CONNECTED,
    NOT_MAC_SENT,
}sock_state;

static char router_mac[50];
static char host_no[50];
static int port_no;
int sockfd = -1;
sock_state state = NOT_INITIALIZED;

static int set_socket_options();
static int _initialize();

int initialize(char *host, int port, char *macaddess);
int read_from_server(char **dest_msg, unsigned short* event_number);
int write_to_server(information* details);

int main()
{
    int initialize(char &host, int port, char &macaddess);

    return 0;
}



if (error != 0) {
    /* socket has a non zero error status */
    fprintf(stderr, "socket error: %s\n", strerror(error));
}

static int set_socket_options()
{
    struct timeval tv_read, tv_write;
    int val = 1;
    tv_read.tv_sec = TIMEOUT;
    tv_read.tv_usec = 0;
    tv_write.tv_sec = TIMEOUT;
    tv_write.tv_usec = 0;

    //return type check
    if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val))<0) //Keep alive TCP
    {
        log_msg(LOG_ERR,"Set Socket option Error: %s\n", strerror(errno));
        return -1;
    }                
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_read, sizeof(struct timeval))<0) //5 sec read timeout
    {
        log_msg(LOG_ERR,"Set socket Option Error: %s\n", strerror(errno));
        return -1;
    }  
    if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv_write, sizeof(struct timeval))<0) //5 second write timeout
    {
        log_msg(LOG_ERR,"Set socket Option Error: %s\n", strerror(errno));
        return -1;
    }

    return 0; 
}

int initialize(char *host, int port, char *macaddess)
{
    port_no = port;
    memset(host_no, 0, sizeof(host_no));
    memset(router_mac, 0, sizeof(router_mac));
    memcpy(host_no, host, 50);         
    memcpy(router_mac, macaddess, 50); 

    return _initialize();
}

static int _initialize()
{
    int mac_result,socket_result;
    struct sockaddr_in servaddr;
    struct hostent *hp;

    switch (state)
    {
    case NOT_INITIALIZED:
    case NOT_CONNECTED:
        if (sockfd > 0)
        {
            close(sockfd);
        }

        sockfd = socket(AF_INET, SOCK_STREAM, 0); //Domain,Type,Protocol

        if (sockfd == -1)
        {
            log_msg(LOG_ERR, "Socket creation failed Error: %s\n", strerror(errno));
            state = NOT_INITIALIZED;
            return -1;
        }

        log_msg(LOG_INFO, "Socket successfully created\n");

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port_no);
        hp = gethostbyname(host_no);
        if (!hp)
        {
            log_msg(LOG_ERR, "Host Error: %s\n", strerror(errno));
            state = NOT_INITIALIZED;
            return -1;
        }

        memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

        //socket_result=set_socket_options(); //set here

        if(socket_result==-1)
        {
            state=NOT_INITIALIZED;
            return -1;
        }

        if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
        {
            log_msg(LOG_ERR, "connection failed Error: %s\n",strerror(errno));
            state = NOT_CONNECTED;
            return -1;
        }
        log_msg(LOG_INFO, "connected to the server\n");
        state = CONNECTED;

     case NOT_MAC_SENT:
         inform.data=(char*)malloc(18*sizeof(char));
         strcpy(inform.data,router_mac);
         inform.data_len=17;
         inform.version_number='1';
         inform.options='0';
         inform.payload_size=21;
         inform.event_number=6;
         mac_result = write_to_server(&inform);

         if (mac_result == -1)
         {
             log_msg(LOG_ERR, "mac address cant send to server\n");
             state = NOT_MAC_SENT;
             return -1;
         }
         log_msg(LOG_MSG, "mac address send successfully\n");
        
        return 0;
    }

    return 0; // 0 success   -1 fail
}

int read_event(char **dest_msg, unsigned short* event_number)
{
    /*
    4 bytes - payload size (version_number+options+event_number+data)
    1 byte - version number
    1 byte - options
    2 bytes - event_number
    based on msg size -data
    */
    int buffer_size,timeout=120,total_size=0,target_size=4,payload_size,result;
    char *rest_buffer,buffer[4];

    signal(SIGPIPE, sigpipe_handler);

    if(state!=CONNECTED)
    {
        result=_initialize();

        if(result==-1)
        {
            log_msg(LOG_INFO,"Reinitialize Error\n");
            return -1;
        }
    }
    
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));

        buffer_size = read(sockfd, buffer, target_size);//Read documrnt

        if(buffer_size<=0)
        {
            if(errno==EINTR)
            {
                log_msg(LOG_INFO,"Signal Interrupt error: %s\n",strerror(errno));
                continue;
            }

            if(errno==EAGAIN && timeout)
            {
                log_msg(LOG_INFO,"Reconnecting again\n");
                timeout=timeout-5;
                continue;
            }
            else
            {
                log_msg(LOG_INFO,"120 sec Read Timeout Done\n");
                return -1;
            }

            log_msg(LOG_INFO, "Socket Error : %s\n", strerror(errno));
            state=NOT_INITIALIZED;
            return -1;
        }

        total_size=total_size+buffer_size;

        if(total_size>=target_size)
        {

            *((char*)&payload_size) = buffer[3];
            *((char*)&payload_size+1) = buffer[2];
            *((char*)&payload_size+2) = buffer[1];
            *((char*)&payload_size+3) = buffer[0];

            rest_buffer=(char*)malloc(payload_size*sizeof(char));

            total_size=0;

            while(1)
            {
                memset(rest_buffer, 0, sizeof(rest_buffer));

                buffer_size = read(sockfd, rest_buffer, payload_size);

                if(buffer_size<=0)
                {
                    if(errno==EINTR)
                    {
                        log_msg(LOG_INFO,"Trying to reconnect error: %s\n",strerror(errno));
                        continue;
                    }

                    if(errno==EAGAIN && timeout)
                    {
                        log_msg(LOG_INFO,"Reconnecting again\n");
                        timeout=timeout-5;
                        continue;
                    }
                    else
                    {
                        log_msg(LOG_INFO,"120 sec Read Timeout Done\n");
                        return -1;
                    }

                    log_msg(LOG_INFO, "Socket Error : %s\n", strerror(errno));
                    state=NOT_INITIALIZED;
                    return -1;
                }

                total_size=total_size+buffer_size;

                if(total_size>=payload_size)
                {
                    break;
                }
            }

            *((char*)&(*event_number)) = rest_buffer[3];
            *((char*)&(*event_number)+1) = rest_buffer[2];

            *dest_msg=rest_buffer+4;

            return 0;
        }
    }

    return -1;
}

int write_event(information* details)
{
    int size,buffer_size,timeout=120,total_buffer_size=0,result;

    typedef unsigned char byte;
    size=details->payload_size+4;
    byte* buffer=(byte*)malloc(size*sizeof(byte));


    buffer[3]= details->payload_size & 0xFF;
    buffer[2]= (details->payload_size>>8) & 0xFF;
    buffer[1]= (details->payload_size>>16) & 0xFF;
    buffer[0]= (details->payload_size>>24) & 0xFF;

    buffer[4]= details->version_number;
    buffer[5]= details->options;

    buffer[6]=(details->event_number >> 8) & 0xFF;
    buffer[7]= details->event_number & 0xFF;

    memcpy(&buffer[8], details->data, details->data_len);

    signal(SIGPIPE, sigpipe_handler);

    if(state!=CONNECTED)
    {
        result=_initialize();

        if(result==-1)
        {
            log_msg(LOG_INFO,"Reinitialize Error\n");
            return -1;
        }
    }

    while(1)
    {
        buffer_size = write(sockfd, buffer, size);

        if(buffer!=NULL)
        {
            free(buffer);
        }

        if(buffer_size<=0)
        {
            if(errno==EINTR)
            {
                log_msg(LOG_INFO,"Trying to reconnect error: %s\n",strerror(errno));
                continue;
            }

            if(errno==EAGAIN && timeout)
            {
                log_msg(LOG_INFO,"Reconnecting again\n");
                timeout=timeout-5;
                continue;
            }
            else
            {
                log_msg(LOG_INFO,"120 sec Read Timeout Done\n");
                return -1;
            }

            log_msg(LOG_INFO, "Socket Error : %s\n", strerror(errno));
            state=NOT_INITIALIZED;
            return -1;
        }

        total_buffer_size=total_buffer_size+buffer_size;

        if(total_buffer_size>=size)
        {
            log_msg(LOG__ERR,"messenge sent successfully\n");
            break;
        }
    }

    return 0;
}
