//gcc -o database_connection database_connection.c -I../instantclient/sdk/include -L../instantclient -lclntsh -Wl,-rpath=../instantclient

// gcc -o server server.c database_connection.c -I../instantclient/sdk/include -L../instantclient -lclntsh -Wl,-rpath=../instantclient




#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "database_connection.h"


#define PORT 8080
#define BUFFER_SIZE 1024

volatile sig_atomic_t stop = 0;

// OCI Variables
OCIEnv *envhp;
OCIServer *srvhp;
OCISession *usrhp;
OCISvcCtx *svchp;
OCIError *errhp;
sword status;

void signal_handler(int sig) {
    // Clean up the OCI environment
    printf("Cleaning up OCI environment...\n");
    clean_up(&usrhp, &svchp, &srvhp, &errhp, &envhp);
    disconnect_from_db(usrhp, svchp, srvhp, errhp, envhp);
    stop = 1;
}


const char *html_page = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                        "<!DOCTYPE html>"
                        "<html>"
                        "<head>"
                        "<title>Autonomous DB C Server</title>"
                        "<style>"
                        "  body {"
                        "    font-family: Arial, sans-serif;"
                        "    background-color: #f0f0f0;"
                        "  }"
                        "  h1 {"
                        "    color: #333;"
                        "  }"
                        "  form {"
                        "    background-color: white;"
                        "    padding: 20px;"
                        "    border-radius: 5px;"
                        "    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);"
                        "  }"
                        "  input[type='text'] {"
                        "    width: 100%;"
                        "    padding: 10px;"
                        "    border: 1px solid #ccc;"
                        "    border-radius: 5px;"
                        "    margin-bottom: 10px;"
                        "  }"
                        "  button[type='submit'] {"
                        "    background-color: #007bff;"
                        "    color: white;"
                        "    padding: 10px;"
                        "    border: none;"
                        "    border-radius: 5px;"
                        "    cursor: pointer;"
                        "  }"
                        "</style>"
                        "</head>"
                        "<body>"
                        "<h1>Enter an SQL statement:</h1>"
                        "<form action=\"/\" method=\"POST\">"
                        "<input type=\"text\" name=\"input_text\">"
                        "<br>"
                        "<button type=\"submit\">Submit</button>"
                        "</form>"
                        "</body>"
                        "</html>";


void handle_post_request(char *request, char *post_data) {
    char *post_data_start = strstr(request, "\r\n\r\n");
    if (post_data_start != NULL) {
        strcpy(post_data, post_data_start + 4); // Skip the "\r\n\r\n" delimiter
    } else {
        post_data[0] = '\0';
    }
    execute_sql_query(status, envhp, svchp, errhp, srvhp, usrhp, post_data);
}

int main() {
    // Server Variables
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char post_data[BUFFER_SIZE] = {0};
    char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 24\r\n\r\nHello from the C server!\n";

    // Initialize the OCI environment
    if (initialize(&envhp, &srvhp, &usrhp, &svchp, &errhp, &status) != 0) {
        printf("Error initializing OCI environment.\n");
        return 1;
    }

    if(status != OCI_SUCCESS) {
        fprintf(stderr, "Failed to initialize the OCI environment.\n");
        return 1;
    }
    // execute sql statement
    printf("Starting Server...\n");
    // sleep(5);




    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, signal_handler);

    // Register the signal handler for SIGINT using sigaction
    // Catch Ctrl+C
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // Accept connections from clients
    while (!stop) {
        printf("\nWaiting for a connection...\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Handle client requests
        valread = read(new_socket, buffer, BUFFER_SIZE);
        printf("Received: %s\n", buffer);

        if (strncmp(buffer, "GET", 3) == 0) {
            // Serve the HTML page
            send(new_socket, html_page, strlen(html_page), 0);
        } else if (strncmp(buffer, "POST", 4) == 0) {
            handle_post_request(buffer, post_data);
            // printf("POST data: %s\n", post_data);
            memset(post_data, 0, BUFFER_SIZE); // Clear the post_data buffer
            //m return a message to the socket
            const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nRecieved by Server. \n";
            send(new_socket, response, strlen(response), 0);
        } else {
            // Send an error message
            char *error_message = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 15\r\n\r\nBad Request\n";
            send(new_socket, error_message, strlen(error_message), 0);
        }

        // Close the connection
        close(new_socket);
        memset(buffer, 0, BUFFER_SIZE);
    }

    printf("Stopping server...\n");
    sleep(5);
    // Clean up the OCI environment
    // seg fault here
    clean_up(&usrhp, &svchp, &srvhp, &errhp, &envhp);

    printf("Disconnectinng\n");
    disconnect_from_db(usrhp, svchp, srvhp, errhp, envhp);

    return 0;
}
