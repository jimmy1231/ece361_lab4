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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#define DEBUG(fmt, args...) (printf(fmt, ##args))

/*
 * 
 */
int main(int argc, char** argv) {
//    int server_addr = atoi(argv[1]);
//    int server_port = atoi(argv[2]);
//    char *user_name = argv[3];
//    DEBUG("Config Parameters:\n   Server Address: %d\n   Server Port: %d\n   User Name: %s\n", server_addr, server_port, user_name);
    
    // create a socket 
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // connect to server
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    int connection_status = connect(client_socket, (struct sockaddr *) &server_address, sizeof (server_address));

    // check for error with the connection
    if (connection_status == -1) {
        DEBUG("THERE was an error making a connection to the remote socket \n\n");
    }
    
    DEBUG("Connection Established!\n");
    
    while (1) {
        char msg[256];
        printf("Enter message you want to send (Max. 128 chars):");
        fgets(msg, 256, stdin);
        DEBUG("You want to send: %s", msg);
        
        // Send something to server, now that we've got the connection set up
        int send_status = send(client_socket, msg, sizeof(msg), 0);

        while (send_status < 0) DEBUG("Send Error!!!!"); 

        // receive data from the server:
        char server_response[256];
        recv(client_socket, &server_response, sizeof (server_response), 0);

        DEBUG("The server sent the data: %s\n", server_response);
    }
    
    close(client_socket);

    return 0;
}


