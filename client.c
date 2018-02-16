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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#define DEBUG(fmt, args...) (printf(fmt, ##args))

/*
 * 
 */

void setup_sock(int *);
int setup_connection(struct sockaddr_in *, int);
void get_command(char *);
int parse_command(char *);

int main(int argc, char** argv) {
//    int server_addr = atoi(argv[1]);
//    int server_port = atoi(argv[2]);
//    char *user_name = argv[3];
//    DEBUG("Config Parameters:\n   Server Address: %d\n   Server Port: %d\n   User Name: %s\n", server_addr, server_port, user_name);
    
    // create a socket 
    int client_socket;
    setup_sock(&client_socket);
    
    // connect to server
    struct sockaddr_in server_address;
    int connection_status = setup_connection(&server_address, client_socket);
    
    // check for error with the connection
    if (connection_status == -1) {
        DEBUG("There was an error making a connection to the remote socket \n\n");
    } else {
        DEBUG("Connection Established!\n");

        while (1) {
            char command[256];
            
            get_command(command);
            int status = parse_command(command);

            if (!status) break; 
            
            // OTHER THINGS
            DEBUG("You want to send: %s, Size: %d\n", command, sizeof(command));

            // Send something to server, now that we've got the connection set up
            int send_status = send(client_socket, command, sizeof(command), 0);

            while (send_status < 0) DEBUG("Send Error!!!!"); 

            // receive data from the server:
            char server_response[256];
            recv(client_socket, &server_response, sizeof (server_response), 0);

            DEBUG("The server sent the data: %s\n", server_response);
        }
    }
    shutdown(client_socket, 2);

    return 0;
}

void setup_sock(int *client_socket) {
    *client_socket = socket(AF_INET, SOCK_STREAM, 0);
}

int setup_connection(struct sockaddr_in * server_address, int client_socket) {
     // connect to server
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(9002);
    server_address->sin_addr.s_addr = INADDR_ANY;

    return connect(client_socket, server_address, sizeof (*server_address));
}

void get_command(char *command) {
    printf("Enter message you want to send (Max. 128 chars):");
    fgets(command, sizeof(command), stdin);
    
    // fgets adds \n to each read character, this line removes that
    command[strcspn(command, "\n")] = '\0';
}

int is_exit(char *command) {
    char exit_str[256] = "exit";
    return (strcmp(command, exit_str) == 0);
}

int is_broadcast(char *command) {
    
}

void handle_broadcast(char **params) {
    
}

void handle_pmessage(char **params) {
    
}

/*
 * is_exit(): Returns 0 if True
 * is_broadcast(): 
 *  
 */

int parse_command(char *command) {
        
    char *cmd = strtok(command, ' ')[0];
    DEBUG("Command: %s", cmd);

    if (is_exit(cmd)) {
        return 0; 
    }
    else if (is_broadcast(cmd)) {
        
    }
    else {
        return 1;
    }
    
}
