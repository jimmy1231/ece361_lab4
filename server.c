/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   main.c
 * Author: lijing53
 *
 * Created on February 15, 2018, 1:39 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#define DEBUG(fmt, args...) (printf(fmt, ##args))
/*
 *
 */

void setup_sock_bind(int *, int);
void poll_for_client_connection(int *, int);

int main(int argc, char** argv) {
//    DEBUG("arg[1]: %s\n", argv[1]);

    // Server Bootstrap
    int server_socket;
    setup_sock_bind(&server_socket, 9002);
    listen(server_socket, 5);

    // Wait for client to connect
    int client_socket;
    poll_for_client_connection(&client_socket, server_socket);

    // No implementation of server yet, this code is to redundantly authenticate users who
    // attempt an authentication request (if reply is AUTH then client will know they
    // are authenticated), keep in mind this is AFTER the TCP 3-Way handshake
    while (1) {
        // Wait
        char recv_message[256];
        int recv_size = recv(client_socket, &recv_message, sizeof (recv_message), 0);
        DEBUG("Received authentication request from client, Size: %d, Content: %s\n", recv_size, recv_message);

        // Send AUTH back to client
        char server_message[256] = "AUTH";
        send(client_socket, server_message, sizeof (server_message), 0);
    }

    // Graceful close, Flag=2 means stop sending and receiving messages from this connection
    shutdown(server_socket, 2);
    return 0;
}

void setup_sock_bind(int *server_socket, int port_num) {
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    unsigned long s_addr = 1000;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_num);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(*server_socket, (struct sockaddr*) &server_address, sizeof (server_address));
//    printf("Hello here!");

}

void poll_for_client_connection(int* client_socket, int server_socket) {

    // Wait state for server to receive client connection
    *client_socket = accept(server_socket, NULL, NULL);

    DEBUG("Connection has been established from: %d\n");

}
