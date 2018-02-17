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
#include <pthread.h>

#define AUTHENTICATE 2
#define BROADCAST 3
#define PRIVATE 4
#define LIST 5
#define MSG_SIZE 256
#define CONNECTION_TIMEOUT 5
#define PTHREAD_WAIT 40000000

#define DEBUG(fmt, args...) (printf(fmt, ##args))

/*
 *
 */

void setup_sock();
int setup_connection(struct sockaddr_in *, int, char *);
void get_command(char []);
int parse_command(char []);

int is_exit(char *);
int is_broadcast(char *);
void handle_broadcast();
int send_to_server(int, char *);
void handle_pmessage();
void print_response(char *);

void format_message(int, char *, char *);
int authenticate(char *);

void *listen_to_server();

int client_socket;
int auth = -1;
int ready = 0;

int main(int argc, char** argv) {
   // int server_addr = atoi(argv[1]);
   // int server_port = atoi(argv[2]);
   char *mock_username = argv[1];
    // char *mock_username = "Jimmy";
//    DEBUG("Config Parameters:\n   Server Address: %d\n   Server Port: %d\n   User Name: %s\n", server_addr, server_port, user_name);

    pthread_t listen;
    // create a socket
    setup_sock();

    // connect to server
    struct sockaddr_in server_address;

    int x = 0;
    if (pthread_create(&listen, NULL, listen_to_server, &x)) {
      DEBUG("Error Creating Thread\n");
    }

    int connection_status = setup_connection(&server_address, client_socket, mock_username);

    // check for error with the connection
    if (connection_status == -1) {
        DEBUG("There was an error making a connection to the remote socket, exiting... \n\n");
        pthread_cancel(listen);
    } else {
        DEBUG("Connection Established! Welcome %s\n", mock_username);

        while (1) {
            char command[MSG_SIZE];

            get_command(command);
            if (!parse_command(command)) break;

            // Wait for listen thread to print out message..
            int i = 0;
            while (++i < PTHREAD_WAIT);
        }
    }

    pthread_cancel(listen);

    // Graceful close, Flag=2 means stop sending and receiving messages from this connection
    shutdown(client_socket, 2);
    return 0;
}

void *listen_to_server(void *ptr) {
  DEBUG("HI WE'RE IN A THREAD!!");

  while (1) {
    char server_response[256];
    int status = recv(client_socket, &server_response, sizeof (server_response), 0);
    if (status < 0) return NULL;

    DEBUG("\nPTHREAD: ");
    print_response(server_response);

    char auth_msg[256] = "AUTHENTICATE AUTH";
    if (strstr(server_response, "AUTHENTICATE")) {
      if (strcmp(server_response, auth_msg) != 0) {
          printf("Authentication Failure: %s\n", (char *)server_response);
          ready = 1;
          auth = 0;
      }
      else {
        DEBUG("Connection successful. Server Response: %s\n\n", server_response);
        ready = 1;
        auth = 1;
      }
    }
  }

  return NULL;
}

void setup_sock() {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
}

int authenticate(char *username) {
    DEBUG("Unauthenticated connection to server successful, attempting to authenticate user: %s\n", username);

    char server_response[MSG_SIZE];
    char auth_msg[256] = "";
    format_message(AUTHENTICATE, auth_msg, username);
    DEBUG("Send AUTHENTICATION message: %s\n", auth_msg);

    int send_attempts = 0;
    while (send(client_socket, auth_msg, sizeof(auth_msg), 0) < 0 && send_attempts < CONNECTION_TIMEOUT) {
        DEBUG("Send Failure, trying again...\n");
        send_attempts++;
    }

    DEBUG("Authentication resend attempts: %d\n", send_attempts);
    if (send_attempts == CONNECTION_TIMEOUT) return -1;

    // recv(client_socket, &server_response, sizeof (server_response), 0);

    // Parse Server response to see if authenticated
    // char auth[256] = "AUTH";
    while (ready == 0 || auth < 0);
    if (auth) return 1;
    else return 0;

}

int setup_connection(struct sockaddr_in * server_address, int client_socket, char *username) {
     // connect to server
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(9002);
    server_address->sin_addr.s_addr = INADDR_ANY;

    int connection_status = connect(client_socket, (struct sockaddr *) server_address, sizeof (*server_address));

    if (connection_status >= 0) {
        /* Do User-Server Authentication: try attempting pinging server for authentication of username
         *
         *  (timeout on 5 attempts), then interpret response, if AUTH, then username exists, if not
         *  it is an invalid user
        */
        if (authenticate(username))
          return 1;
        else
          return -1;
    }
    else {
        DEBUG("Failed to connect to server\n");
        return -1;
    };
}

void get_command(char command[MSG_SIZE]) {
    printf("Enter message you want to send (Max. 256 chars): ");
    fgets(command, MSG_SIZE, stdin);
    printf("\n");

    // fgets adds \n to each read character, this line removes that
    command[strcspn(command, "\n")] = '\0';
}

int is_exit(char *command) {
    char exit_str[MSG_SIZE] = "exit";
    return (strcmp(command, exit_str) == 0);
}

int is_broadcast(char *command) {
    char broadcast_str[MSG_SIZE] = "broadcast";
    return (strcmp(command, broadcast_str) == 0);
}

void handle_broadcast() {

    char message[MSG_SIZE] = "";
    char *token = strtok(NULL, " ");

    while (token != NULL) {
        strcat(message, token);
        strcat(message, " ");

        token = strtok(NULL, " ");
    }

    send_to_server(BROADCAST, message);
}

void handle_pmessage() {
    char *username = strtok(NULL, " ");
}

/*
 * is_exit(): Returns 0 if True
 * is_broadcast():
 *
 * Quick Note on strtok: everytime we call it truncates the original string
 */

int parse_command(char command[MSG_SIZE]) {

    char *cmd = strtok(command, " ");

    if (is_exit(cmd)) {
        return 0;
    }
    else if (is_broadcast(cmd)) {
        handle_broadcast();
        return 1;
    }
    else {
        handle_pmessage();
        return 1;
    }
}

int send_to_server(int type, char *body) {
    char server_response[MSG_SIZE];

    if (type == BROADCAST) {
        char broadcast_message[256] = "";
        format_message(BROADCAST, broadcast_message, body);
        DEBUG("BROADCAST message: %s\n", broadcast_message);

        int send_status = send(client_socket, broadcast_message, MSG_SIZE, 0);

        if (send_status > 0) {
            // receive data from the server:
            // recv(client_socket, &server_response, sizeof (server_response), 0);
            // print_response(type, server_response);
        } else {
            DEBUG("[BROADCAST]: SOMETHING WENT WRONGGGGG :(");
        }
    }
    else if (type == PRIVATE) {
        DEBUG("Sending PRIVATE message: %s\n", body);
    }
    else if (type == LIST) {
        DEBUG("Sending LIST request\n");
    }
}

void print_response(char *server_response) {
    printf("Server Sent: %s\n\n---------------------------------------------------------\n\n", server_response);
}

void format_message(int type, char *message, char *body) {
    if (type == AUTHENTICATE) {
      strcat(message, "AUTHENTICATE ");
      strcat(message, body);
    }
    else if (type == BROADCAST){
      strcat(message, "BROADCAST ");
      strcat(message, body);
    }
    else if (type == PRIVATE) {
      strcat(message, "PRIVATE ");
      strcat(message, body);
    }
    else if (type == LIST) {
      strcat(message, "LIST");
    }
}
