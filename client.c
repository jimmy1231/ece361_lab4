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

void setup_sock();
int setup_connection(struct sockaddr_in *, int, int, char *);
void get_command(char []);
int parse_command(char []);

int is_exit(char *);
int is_broadcast(char *);
int is_pmessage(char *);
int is_list(char *);
void handle_broadcast();
int send_to_server(int, char *);
void handle_pmessage();
void handle_list();
void print_response(int, char *);
int get_message_command(char *, char *);

void format_message(int, char *, char *);
int authenticate(char *);

void *listen_to_server();

int client_socket;
int auth = -1;
int ready = 0;


int main(int argc, char** argv) {
   int server_addr = atoi(argv[1]);
   int server_port = atoi(argv[2]);
   char *mock_username = argv[3];
   DEBUG("Config Parameters:\n   Server Address: %d\n   Server Port: %d\n   User Name: %s\n\n", server_addr, server_port, mock_username);

    pthread_t listen;
    struct sockaddr_in server_address;

    setup_sock();
    pthread_create(&listen, NULL, listen_to_server, NULL);
    int connection_status = setup_connection(&server_address, server_port, client_socket, mock_username);

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
    shutdown(client_socket, 2);
    return 0;
}

void *listen_to_server(void *ptr) {
  while (1) {
    char server_response[256] = "";
    char response_body[256] = "";
    int status = recv(client_socket, &server_response, sizeof (server_response), 0);
    if (status < 0) return NULL;

    DEBUG("\nPTHREAD: ");

    char auth_msg[256] = "AUTHENTICATE AUTH";
    if (strstr(server_response, "AUTHENTICATE")) {
      if (strcmp(server_response, auth_msg) != 0) {
          printf("Authentication Failure: %s\n", (char *)server_response);
          ready = 1;
          auth = 0;
      } else {
        DEBUG("Connection successful. Server Response: %s\n\n", server_response);
        ready = 1;
        auth = 1;
      }
    } else if (strstr(server_response, "PRIVATE ERROR")) {
      printf("PRIVATE MESSAGE ERROR: The user you are trying to message is not registered.\n");
    }
    else {
      DEBUG("Server Raw Response: %s\n", server_response);
      int type = get_message_command(server_response, response_body);
      print_response(type, response_body);
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

    if (send(client_socket, auth_msg, sizeof(auth_msg), 0) < 0)
      DEBUG("Authentication request failed to send, exiting...\n");

    // Parse Server response to see if authenticated
    while (ready == 0 || auth < 0);
    if (auth) return 1;
    else return 0;
}

int setup_connection(struct sockaddr_in * server_address, int server_socket, int client_socket, char *username) {
     // connect to server
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons(server_socket);
    server_address->sin_addr.s_addr = INADDR_ANY;

    int connection_status = connect(client_socket, (struct sockaddr *) server_address, sizeof (*server_address));

    if (connection_status >= 0) {
        if (authenticate(username))
          return 1;
        else
          return -1;
    } else {
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
    char broadcast_str[MSG_SIZE] = "BROADCAST";
    return (strcmp(command, broadcast_str) == 0);
}

int is_pmessage(char *command) {
    char pmessage_str[MSG_SIZE] = "PRIVATE";
    return (strcmp(command, pmessage_str) == 0);
}

int is_list(char *command) {
    char list_str[MSG_SIZE] = "LIST";
    return (strcmp(command, list_str) == 0);
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
    char message[MSG_SIZE] = "";
    char *token = strtok(NULL, " ");

    while (token != NULL) {
        strcat(message, token);
        strcat(message, " ");

        token = strtok(NULL, " ");
    }
    send_to_server(PRIVATE, message);
}

void handle_list() {
    send_to_server(LIST, NULL);
}

int parse_command(char command[MSG_SIZE]) {
    char *cmd = strtok(command, " ");

    if (is_exit(cmd)) {
        return 0;
    } else if (is_broadcast(cmd)) {
        handle_broadcast();
        return 1;
    } else if (is_pmessage(cmd)){
        handle_pmessage();
        return 1;
    } else if (is_list(cmd)){
        handle_list();
        return 1;
    }
}

int send_to_server(int type, char *body) {
    char server_response[MSG_SIZE];
    char send_message[256] = "";
    format_message(type, send_message, body);

    DEBUG("Client Request: %s\n", send_message);
    int send_status = send(client_socket, send_message, MSG_SIZE, 0);

    if (type == BROADCAST)
        if (send_status < 0)
          DEBUG("[BROADCAST]: SOMETHING WENT WRONGGGGG :(");
    else if (type == PRIVATE)
        if (send_status < 0)
          DEBUG("[PRIVATE]: SOMETHING WENT WRONGGGG :(");
    else if (type == LIST)
        if (send_status < 0)
          DEBUG("[LIST]: SOMETHING WENT WRONGGGG :(");
}

void print_response(int type, char *server_response) {
    printf("\nResponse from server: \n---------------------------------------------------------\n\n");

    if (type == BROADCAST)
    {
      char *username = strtok(server_response, " ");
      char *token = strtok(NULL, " ");
      char message[MSG_SIZE] = "";

      while (token != NULL) {
        strcat(message, token);
        strcat(message, " ");
        token = strtok(NULL, " ");
      }
      printf("Broadcast message from: %s\n", username);
      printf("[Content]: %s\n", message);
    }
    else if (type == PRIVATE)
    {
      char *username = strtok(server_response, " ");
      char *token = strtok(NULL, " ");
      char message[MSG_SIZE] = "";

      while (token != NULL) {
        strcat(message, token);
        strcat(message, " ");
        token = strtok(NULL, " ");
      }
      printf("Private message from %s: %s\n", username, message);
    }
    else if (type == LIST)
    {
      printf("List of active users: \n");
      char *token = strtok(server_response, " ");
      while (token != NULL) {
        printf("   * %s\n", token);
        token = strtok(NULL, " ");
      }
    }
    printf("\n---------------------------------------------------------\n\n");
}

int get_message_command(char *message, char *body) {
    char *cmd = strtok(message, " ");
    char *token = strtok(NULL, " ");

    while (token != NULL) {
        strcat(body, token);
        strcat(body, " ");
        token = strtok(NULL, " ");
    }

    if (is_broadcast(cmd))
      return BROADCAST;
    else if (is_pmessage(cmd))
      return PRIVATE;
    else if (is_list(cmd))
      return LIST;
    else
      return 0;
}

void format_message(int type, char *message, char *body) {
    if (type == AUTHENTICATE) {
      strcat(message, "AUTHENTICATE ");
      strcat(message, body);
    }
    else if (type == BROADCAST)
    {
      strcat(message, "BROADCAST ");
      strcat(message, body);
    }
    else if (type == PRIVATE)
    {
      strcat(message, "PRIVATE ");
      strcat(message, body);
    }
    else if (type == LIST)
    {
      strcat(message, "LIST");
    }
}
