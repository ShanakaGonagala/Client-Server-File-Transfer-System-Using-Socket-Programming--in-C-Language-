
# Client-Server File Transfer System Using Socket Programming (in C Language)


This project implements a basic client-server file transfer system using C socket programming. It provides functionality for multiple clients to connect to a server, download files from a shared directory, or upload files for others to access. The system also logs all client activities, including file transfers, to maintain a complete record of operations.

## Features

- **File Download**: Clients can download files from a predefined directory on the server, simulating torrent file downloading.
- **File Upload**: Clients can upload files to the server, mimicking torrent file sharing.
- **Logging**: The server logs all client interactions such as file transfers, connections, and disconnections.


## Directory Structure

Make sure the following directories are created on the server and client machines before running the application:

### Server
The server-side runs on one machine and has the following structure:

```bash
server/
├── client_uploads/      # Directory for uploaded files on the server
├── shared/              # Folder containing files that clients can download
└── source_files/        # Directory containing server source code and log file
    ├── srv4956.c        # Server-side code
    └── log_srv4956.txt  # Server log file (generated during execution)
    
```
### Client

The client-side runs on another machine with the following structure:

```bash
client/
├── uploads/             # Directory for files to upload from the client
├── downloads/           # Directory for downloaded files on the client
└── cli2231.c            # Client-side code

```
## How to Compile and Run

### Server Side

1.  **Compile the Server Code**: On the server machine, navigate to the `source_files` directory and run the following command to compile:

    ```bash    
    gcc -o server srv4956.c
    ```
2.  **Run the Server**: After compiling, run the server program:
    
    ```bash
    ./server
    ```
    This will start the server on a predefined port (e.g., 2231) and wait for client connections.
    

### Client Side

1.  **Compile the Client Code**: On the client machine, navigate to the client directory and run the following command to compile:
    
    ```bash
    gcc -o client cli2231.c 
    ```
2.  **Run the Client**: Run the client by specifying the server’s IP address:
    
    ```bash
    ./client <server_ip>
    ```
    Replace `<server_ip>` with the actual IP address of the server.

## Logs and File Tracking

The server maintains a log of client activities, which includes:

-   Client IP and port
-   Connection and disconnection timestamps
-   Files downloaded or uploaded
-   Success or failure status of file transfers

The logs are saved in the `log_srv4956.txt` file.

## Example Run

### Server Example:

```bash
$ ./server
Server started at IP: 192.168.1.5, Port: 2231
Waiting for client connections...
```
### Client Example:

#### Downloading a File:
```bash
$ ./client 192.168.1.5
Connected to server at 192.168.1.5:2231

[1] I want to DOWNLOAD a file.
[2] I want to UPLOAD a file.

Choose an option [1/2]: 1

Available files:
    [1] file1.txt
    [2] file2.pdf
Enter the file number to download: 1
Downloading file1.txt...
[##########--------------------] 50% 
Download complete.
```
#### Uploading a File:

```bash
$ ./client 192.168.1.5
Connected to server at 192.168.1.5:2231

[1] I want to DOWNLOAD a file.
[2] I want to UPLOAD a file.

Choose an option [1/2]: 2

Available files to upload:
    [1] upload1.jpg
    [2] report.pdf
    [3] song.mp3
Enter the file number to upload: 2
Uploading uploads/report.pdf...
[##########--------------------] 50% 
Upload complete: uploads/report.pdf
```

### Log Output

```
----------Client Session----------
Client IP: 192.168.1.4, Port: 36006
Connected: Mon Sep 30 00:56:32 2024
File: file1.txt, Status: Download completed
Disconnected: Mon Sep 30 00:57:34 2024
-------------------------------
----------Client Session----------
Client IP: 192.168.1.4, Port: 35988
Connected: Mon Sep 30 01:33:25 2024
File: report.pdf, Status: Upload completed
Disconnected: Mon Sep 30 01:33:44 2024
-------------------------------
```
