#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 2231
#define CHUNK_SIZE 1024
#define BASE_DIR "/home/it22314956/shared"
#define UPLOAD_DIR "/home/it22314956/client_uploads"
#define LOG_FILE "log_srv4956.txt"
#define server_ip "192.168.110.216"

void handle_client(int client_socket, struct sockaddr_in client_addr);
void send_file(int client_socket, const char *filename);
void receive_file(int client_socket, const char *filename);
void log_client_activity(const char *log_info);
int get_sorted_file_list(char ***file_list);
void free_file_list(char **file_list, int count);

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pid_t child_pid;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the server address structure using memset
    memset(&server_addr, 0, sizeof(server_addr));

    // Bind socket to IP and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_socket, 100) < 0) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    printf("\nServer started at IP: %s, Port: %d\n\nWaiting for client connections...\n", server_ip, PORT);
    
    while (1) {
        // Accept incoming connection
        memset(&client_addr, 0, sizeof(client_addr));  // Clear client address structure before each connection
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket < 0) {
            perror("Failed to accept connection");
            exit(EXIT_FAILURE);
        }

        // Create child process to handle each client
        if ((child_pid = fork()) == 0) {
            close(server_socket);
            handle_client(new_socket, client_addr); // Handle client connection
            close(new_socket);
            exit(0);
        }
        close(new_socket); // Parent process closes the socket
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[1024];
    char filename[256];
    char **file_list = NULL;
    int file_count = 0;
    char catalog[4096] = "";
    int file_index, option, i;
    char log_info[2048] = "";  // Accumulate log information
    time_t connect_time, disconnect_time;

    // Clear memory before use
    memset(buffer, 0, sizeof(buffer));
    memset(filename, 0, sizeof(filename));
    memset(catalog, 0, sizeof(catalog));
    
    printf("\nClient connected - IP: %s, Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Log client connection
    connect_time = time(NULL);
    snprintf(log_info, sizeof(log_info), "----------Client Session----------\nClient IP: %s, Port: %d\nConnected: %s",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), ctime(&connect_time));

    // Receive the option from the client (1 for download, 2 for upload)
    if (recv(client_socket, &option, sizeof(option), 0) <= 0) {
        perror("Failed to receive option");
        close(client_socket);
        return;
    }

    if (option == 1) {
        // Handle file download
        file_count = get_sorted_file_list(&file_list);
        strcat(catalog, "Available files:\n");
        for (i = 0; i < file_count; i++) {
            char entry[256];
            snprintf(entry, sizeof(entry), "\t[%d] %s\n", i + 1, file_list[i]);
            strcat(catalog, entry);
        }
        send(client_socket, catalog, strlen(catalog), 0);

        if (recv(client_socket, &file_index, sizeof(file_index), 0) > 0) {
            if (file_index > 0 && file_index <= file_count) {
                snprintf(filename, sizeof(filename), "%s/%s", BASE_DIR, file_list[file_index - 1]);
                send(client_socket, file_list[file_index - 1], strlen(file_list[file_index - 1]) + 1, 0);
                send_file(client_socket, filename);
                snprintf(log_info + strlen(log_info), sizeof(log_info) - strlen(log_info), 
                         "File: %s, Status: Download completed\n", file_list[file_index - 1]);
            } else {
                send(client_socket, "Invalid file index\n", strlen("Invalid file index\n"), 0);
                snprintf(log_info + strlen(log_info), sizeof(log_info) - strlen(log_info), 
                         "File: None, Status: Invalid file index\n");
            }
        }
    } else if (option == 2) {
        // Handle file upload
        if (recv(client_socket, filename, sizeof(filename), 0) <= 0) {
            perror("Failed to receive filename");
            close(client_socket);
            return;
        }
        receive_file(client_socket, filename);
        snprintf(log_info + strlen(log_info), sizeof(log_info) - strlen(log_info), 
                 "File: %s, Status: Upload completed\n", filename);
    }

    // Log disconnection details
    disconnect_time = time(NULL);
    snprintf(log_info + strlen(log_info), sizeof(log_info) - strlen(log_info),
             "Disconnected: %s-------------------------------\n", ctime(&disconnect_time));

    // Log everything at once
    log_client_activity(log_info);

    free_file_list(file_list, file_count);
}

void send_file(int client_socket, const char *filename) {
    FILE *file;
    char buffer[CHUNK_SIZE];
    int bytes_read;
    struct stat file_stat;

    // Get the file size using stat
    if (stat(filename, &file_stat) < 0) {
        perror("Failed to get file size");
        send(client_socket, "ERROR", strlen("ERROR"), 0);
        return;
    }
    size_t file_size = file_stat.st_size;

    printf("Sent file size: %zu bytes\n", file_size);

    // Send the file size to the client
    if (send(client_socket, &file_size, sizeof(file_size), 0) < 0) {
        perror("Failed to send file size");
        return;
    }

    // Open the file for reading in binary mode
    file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        send(client_socket, "ERROR", strlen("ERROR"), 0);
        return;
    }

    // Send the file content in chunks
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            perror("Failed to send file");
            break;
        }
    }

    fclose(file);
    printf("File sent: %s\n", filename);
}

void receive_file(int client_socket, const char *filename) {
    FILE *file;
    char buffer[CHUNK_SIZE];
    int bytes_received;
    ssize_t total_bytes_received = 0;

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", UPLOAD_DIR, filename);

    file = fopen(filepath, "wb"); // Open in binary mode
    if (!file) {
        perror("Failed to create file");
        send(client_socket, "Failed to create file\n", strlen("Failed to create file\n"), 0);
        return;
    }

    printf("Receiving file %s...\n", filepath);

    while ((bytes_received = recv(client_socket, buffer, CHUNK_SIZE, 0)) > 0) {
        // Check for end-of-file marker
        if (strncmp(buffer, "EOF", 3) == 0) {
            break;
        }

        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;
        printf("\rReceived %zu bytes", total_bytes_received);
        fflush(stdout);
    }

    if (bytes_received < 0) {
        perror("Error receiving file");
    } else {
        printf("\nFile received: %s\n", filepath);
    }

    fclose(file);
}

void log_client_activity(const char *log_info) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        perror("Failed to open log file");
        return;
    }
    fprintf(log_file, "%s", log_info);  // Write the entire log string
    fclose(log_file);
}

int alphanum_compare(const void *a, const void *b) {
    return strcasecmp(*(const char **)a, *(const char **)b);
}

int get_sorted_file_list(char ***file_list) {
    struct dirent **namelist;
    int n = scandir(BASE_DIR, &namelist, NULL, alphasort);
    int i;
    if (n < 0) {
        perror("scandir");
        return 0;
    }
    *file_list = malloc(n * sizeof(char *));
    int count = 0;
    for (i = 0; i < n; i++) {
        if (namelist[i]->d_type == DT_REG) {
            (*file_list)[count] = strdup(namelist[i]->d_name);
            count++;
        }
        free(namelist[i]);
    }
    free(namelist);

    // Sort the file list alphanumerically
    qsort(*file_list, count, sizeof(char *), alphanum_compare);

    return count;
}

void free_file_list(char **file_list, int count) {
    int i;
    for (i = 0; i < count; i++) {
        free(file_list[i]);
    }
    free(file_list);
}
