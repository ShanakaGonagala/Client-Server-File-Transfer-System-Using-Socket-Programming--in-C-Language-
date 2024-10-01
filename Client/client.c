#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#define SERVER_PORT 2231
#define CHUNK_SIZE 1024
#define DOWNLOAD_DIR "downloads"
#define UPLOADING_FILES "uploads"
#define PROGRESS_BAR_WIDTH 50  // Width of the progress bar in characters

void download_file(int socket, const char *filename);
void upload_file(int socket, const char *filename);
int get_sorted_file_list(char ***file_list, const char *directory);
void free_file_list(char **file_list, int count);

int main(int argc, char *argv[]) {
    if (argc != 2) {  // Expect only the server IP as the argument
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_addr;
    char catalog[4096] = {0};
    int file_index;
    char filename[256] = {0};
    int option;
    char **file_list = NULL;
    int file_count = 0;

    // Get the server IP from command-line arguments
    const char *server_ip = argv[1];

    // Zero out the server address structure
    memset(&server_addr, 0, sizeof(server_addr));

    // Define server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        exit(EXIT_FAILURE);
    }

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Print connected server IP and port
    printf("\nConnected to server at %s:%d\n\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    // Ask the user for the option (1 for download, 2 for upload)
    printf("\t[1] I want to DOWNLOAD a file.\n\t[2] I want to UPLOAD a file\n\nChoose an option [1/2]: ");
    scanf("%d", &option);

    // Send the option to the server
    if (send(client_socket, &option, sizeof(option), 0) < 0) {
        perror("Failed to send option");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (option == 1) {
        // Receive and display catalog from server
        if (recv(client_socket, catalog, sizeof(catalog), 0) < 0) {
            perror("Failed to receive catalog");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
        printf("%s", catalog);

        // Ask the user for the file index to download
        printf("Enter the file number to download: ");
        scanf("%d", &file_index);

        // Send the file index to the server
        if (send(client_socket, &file_index, sizeof(file_index), 0) < 0) {
            perror("Failed to send file index");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Receive the file name (for downloading the correct file)
        if (recv(client_socket, filename, sizeof(filename), 0) < 0) {
            perror("Failed to receive file name");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Download the file
        download_file(client_socket, filename);
    } else if (option == 2) {
        // Get sorted list of files in the 'upload_files' directory
        file_count = get_sorted_file_list(&file_list, UPLOADING_FILES);

        // Display the list of files with index numbers
        printf("Available files to upload:\n");
        for (int i = 0; i < file_count; i++) {
            printf("[%d] %s\n", i + 1, file_list[i]);
        }

        // Ask the user for the file index to upload
        printf("Enter the file number to upload: ");
        scanf("%d", &file_index);

        if (file_index > 0 && file_index <= file_count) {
            // Construct the full path to the file in the 'upload_files' folder
            snprintf(filename, sizeof(filename), "%s/%s", UPLOADING_FILES, file_list[file_index - 1]);

            // Send the file name to the server
            if (send(client_socket, file_list[file_index - 1], strlen(file_list[file_index - 1]) + 1, 0) < 0) {
                perror("Failed to send file name");
                close(client_socket);
                exit(EXIT_FAILURE);
            }

            // Upload the file
            upload_file(client_socket, filename);
        } else {
            printf("Invalid file index\n");
        }

        // Free the file list
        free_file_list(file_list, file_count);
    }

    close(client_socket);  // Close connection after one operation

    // Print disconnected status
    printf("Disconnected from server.\n");

    return 0;
}

void print_progress_bar(int percentage) {
    int bar_width = PROGRESS_BAR_WIDTH;
    int pos = (percentage * bar_width) / 100;

    printf("[");
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) {
            printf("#");
        } else {
            printf("-");
        }
    }
    printf("] %d%%\r", percentage);
    fflush(stdout);  // Flush output to display progress bar immediately
}

void download_file(int socket, const char *filename) {
    FILE *file;
    char buffer[CHUNK_SIZE];
    ssize_t bytes_received;
    size_t total_bytes_received = 0;
    size_t file_size;
    int progress = 0;

    // **Receive the file size from the server**
    if (recv(socket, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Failed to receive file size");
        return;
    }
    printf("File size: %zu bytes\n", file_size);

    // Open the file in the downloads folder with the same name
    char filepath[512];
    memset(filepath, 0, sizeof(filepath));
    snprintf(filepath, sizeof(filepath), "%s/%s", DOWNLOAD_DIR, filename);
    file = fopen(filepath, "wb");
    if (!file) {
        perror("Failed to create file");
        return;
    }

    printf("Downloading %s...\n", filename);

    // Download the file in chunks and display progress
    while ((bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;

        // Calculate the new progress percentage
        int new_progress = (int)(((double)total_bytes_received / (double)file_size) * 100);

        // **Print progress bar**
        print_progress_bar(new_progress);

        // Check if the file has been fully downloaded
        if (total_bytes_received >= file_size) {
            break;
        }
    }

    if (bytes_received < 0) {
        perror("Error receiving file");
    } else {
        printf("\nDownload complete. File saved as %s\n", filepath);
    }

    fclose(file);
}

void upload_file(int socket, const char *filepath) {
    FILE *file;
    char buffer[CHUNK_SIZE];
    int bytes_read;
    size_t total_bytes_sent = 0;
    struct stat file_stat;
    int progress = 0;

    // Get the file size
    if (stat(filepath, &file_stat) < 0) {
        perror("Failed to get file size");
        return;
    }
    size_t file_size = file_stat.st_size;

    file = fopen(filepath, "rb"); // Open in binary mode
    if (!file) {
        perror("Failed to open file");
        send(socket, "Failed to open file\n", strlen("Failed to open file\n"), 0);
        return;
    }

    printf("Uploading %s...\n", filepath);

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (send(socket, buffer, bytes_read, 0) < 0) {
            perror("Failed to send file");
            break;
        }
        total_bytes_sent += bytes_read;

        // Calculate the new progress percentage
        int new_progress = (int)(((double)total_bytes_sent / (double)file_size) * 100);

        // Print progress bar
        if (new_progress > progress) { // Update only if progress has changed
            progress = new_progress;
            print_progress_bar(progress);
        }
    }

    if (total_bytes_sent == file_size) {
        printf("\nUpload complete: %s\n", filepath);
    } else {
        printf("\nUpload interrupted: %s\n", filepath);
    }

    fclose(file);
}

int get_sorted_file_list(char ***file_list, const char *directory) {
    struct dirent **namelist;
    int n = scandir(directory, &namelist, NULL, alphasort);
    if (n < 0) {
        perror("scandir");
        return 0;
    }
    *file_list = malloc(n * sizeof(char *));
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (namelist[i]->d_type == DT_REG) {
            (*file_list)[count] = strdup(namelist[i]->d_name);
            count++;
        }
        free(namelist[i]);
    }
    free(namelist);

    return count;
}

void free_file_list(char **file_list, int count) {
    for (int i = 0; i < count; i++) {
        free(file_list[i]);
    }
    free(file_list);
}
