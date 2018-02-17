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

#define AUTHENTICATE 2
#define BROADCAST 3
#define PRIVATE 4
#define LIST 5
#define MSG_SIZE 256
#define MAX_USERS 10

#define DEBUG(fmt, args...) (printf(fmt, ##args))
/*
 *
 */

 typedef struct {
     char * user_name;
     int socket_id;

 } User;

void setup_sock_bind(int *, int);
void poll_for_client_connection(int *, int);

int parse_command(char *, char *);
void handle_request(int, char*, fd_set *, int, int, User **, int, int *);
void handle_broadcast(fd_set *, char*, int, int);
void handle_authenticate(char*, fd_set *, User **, int *, int);

int user_found(User **, char*, int);

int main(int argc, char** argv) {
//    DEBUG("arg[1]: %s\n", argv[1]);

    // Create list of users
    User *users[MAX_USERS];
    int user_id = 0;    // Keep track of how many users are connected

    // Setup fd_sets for concurrency
    fd_set master;
    fd_set read_fds;
    int fdmax;  // Max file descriptor for select()

    FD_ZERO(&master);
    FD_ZERO(&read_fds);


    // Server Bootstrap
    int server_socket;
    setup_sock_bind(&server_socket, 9002);
    listen(server_socket, 5);

    // Add server_socket to master set
    FD_SET(server_socket, &master);

    // keep track of the biggest file descriptor
    fdmax = server_socket;

    // No implementation of server yet, this code is to redundantly authenticate users who
    // attempt an authentication request (if reply is AUTH then client will know they
    // are authenticated), keep in mind this is AFTER the TCP 3-Way handshake
    while (1) {
        read_fds = master;

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
          DEBUG("ERROR IN SELECT\n");
          break;
        }

        // Client socket index
        int c_sock = 0;
        for (c_sock = 0; c_sock <= fdmax; c_sock++) {
          if (FD_ISSET(c_sock, &read_fds)){ // We have a send/rcv request
            if (c_sock == server_socket) {  // The server got a connection request
              // Handle new connections
              int newfd;
              poll_for_client_connection(&newfd, server_socket);

              // Check if connection to newfd was success
              if (newfd == -1) {
                DEBUG("ERROR CONNECTING TO NEW CLIENT!\n\n");
              } else {
                FD_SET(newfd, &master); // Add new socket to master
                if (newfd > fdmax) {  // Change value of fdmax
                  fdmax = newfd;
                }
              }
            } else {  // Handle data from client
              char recv_message[256];
              char server_message[256] = "AUTH";
              char msg_body[256]; //

              // Receive data from client and send AUTH upon success
              int recv_size = recv(c_sock, &recv_message, sizeof (recv_message), 0);
              if (recv_size == -1 ) {
                DEBUG("ERROR RECEIVING MESSAGE FROM %d\n\n", c_sock);
                FD_CLR(c_sock, &master);
              } else {

                /*
                 * cmd: 'BROADCAST', 'AUTHENTICATE', 'PRIVATE', 'LIST'
                 * msg_body: message to be sent
                 */

                int cmd = parse_command(recv_message, msg_body);
                handle_request(cmd, msg_body, &master, fdmax, server_socket, users, c_sock, &user_id);
                strcpy(msg_body, "");

                DEBUG("Received authentication request from client: %d, Content: %s\n", c_sock, recv_message);
              }
            }
          }
        }
    }

    // Graceful close, Flag=2 means stop sending and receiving messages from this connection
    shutdown(server_socket, 2);
    return 0;
}

int parse_command(char * rcv_msg, char * msg_body) {
    char * cmd = strtok(rcv_msg, " ");
    int cmd_num;

    if (strcmp(cmd, "LIST") == 0) return LIST;
    else if (strcmp(cmd, "BROADCAST") == 0) cmd_num = BROADCAST;
    else if (strcmp(cmd, "AUTHENTICATE") == 0) cmd_num = AUTHENTICATE;
    else if (strcmp(cmd, "PRIVATE") == 0) cmd_num = PRIVATE;

    cmd = strtok(NULL, " ");

    // Parse out msg_body
    while (cmd != NULL) {
      strcat(msg_body, cmd);
      strcat(msg_body, " ");
      cmd = strtok(NULL, " ");
    }

    return cmd_num;
}

void handle_request(int cmd, char * msg_body, fd_set * master, int fdmax, int server_socket, User **users, int client_socket, int * user_id) {
  if (cmd == AUTHENTICATE){
      DEBUG("CMD: AUTHENTICATE\nUSER_NAME: %s\n", msg_body);
      handle_authenticate(msg_body, master, users, user_id, client_socket);
  }
  else if (cmd == BROADCAST) {
      handle_broadcast(master, msg_body, fdmax, server_socket);
      DEBUG("CMD: BROADCAST\nBODY: %s\n", msg_body);
  }
  else if (cmd == PRIVATE) {
      DEBUG("CMD: PRIVATE\n");
  }
  else if (cmd == LIST) {
      DEBUG("CMD: LIST\n");
  }
}

void handle_authenticate(char* msg_body, fd_set * master, User ** users, int * user_id, int c_sock){
    char server_message[256] = "AUTH";

    //1. Add user to 'users' array if user_name not found in list of users
    if (*user_id == 0 || !user_found(users, msg_body, *user_id + 1)){

        User * new_user = malloc(sizeof(User));
        users[*user_id] = new_user;
        users[*user_id] -> user_name = msg_body;
        users[*user_id] -> socket_id = c_sock;

        DEBUG("ADDING USER TO 'users': \n");
        DEBUG("user_name: %s  |  socket_id: %d  |  user_count: %d \n\n", users[*user_id] -> user_name, users[*user_id] -> socket_id, *user_id);
        send(c_sock, server_message, sizeof(server_message), 0);

        *user_id += 1;
        // TO-DO: BROADCAST to all connected users
        
    } else {
        printf("AUTHENTICATION ERROR: User_name %s is already taken :( \n\n", msg_body);
        FD_CLR(c_sock, master);
    }
}

int user_found(User ** users, char* user_name, int num_users) {
    int i = 0;
    for (i = 0; i < num_users; i++) {
        if (strcmp(users[i]->user_name, user_name) == 0){
          return 1;
        }
    }
    return 0;
}

/*
 * Sends 'msg_body' to all connected users
 */
void handle_broadcast(fd_set * master, char* msg_body, int fdmax, int server_socket) {
    int i = 0;
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, master)) {
          if (i != server_socket) {
            DEBUG("SENDING '%s' TO: %d\n", msg_body, i);
            if (send(i, msg_body, MSG_SIZE, 0) == -1){
                DEBUG("ERROR SENDING BROADCAST TO: %d\n\n", i);
            }
          }
      }
    }
}

void setup_sock_bind(int *server_socket, int port_num) {
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    unsigned long s_addr = 1000;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_num);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(*server_socket, (struct sockaddr*) &server_address, sizeof (server_address));

}

void poll_for_client_connection(int* client_socket, int server_socket) {

    // Wait state for server to receive client connection
    *client_socket = accept(server_socket, NULL, NULL);

    DEBUG("\nNEW CONNECTION!\nConnection has been established from: %d\n\n", *client_socket);

}
