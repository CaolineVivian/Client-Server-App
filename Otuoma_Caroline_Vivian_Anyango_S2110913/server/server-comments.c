// Cwk2: server.c - multi-threaded server using readn() and writen()

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include "rdwrn.h"


// thread function
void *client_handler(void *);

// Function declarations
void handle_student_info(int connfd, struct sockaddr_in client_addr);
void handle_random_numbers(int connfd);
void handle_uname_info(int connfd);
void handle_file_list(int connfd);
void handle_file_request(int connfd);

// Function to ensure the "upload" directory exists and create initial files
void ensure_upload_directory() {
    // Check if "upload" directory exists
    struct stat st = {0};

    printf("Checking for 'upload' folder");

    // Countdown for checking Upload folder
    int i;
    for (i = 0; i < 3; ++i) {
    	usleep(500000);  // Sleep for 0.5 seconds
    	printf(".");
    	fflush(stdout);  // Flush the output buffer to show dots immediately
    }

    printf("\n");

    if (stat("upload", &st) == -1) {
        // "upload" directory does not exist, create it
        printf("Upload Folder not found. Creating 'upload' folder and saving test files...\n");
        mkdir("upload", 0700);
	
        FILE *file1 = fopen("upload/vivian", "w");
        FILE *file2 = fopen("upload/test1.txt", "w");
        FILE *file3 = fopen("upload/test2.txt", "w");

        if (file1 && file2 && file3) {
            fclose(file1);
            fclose(file2);
            fclose(file3);
            printf("Test files saved successfully.\n");
        } else {
            perror("Error creating initial files");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Upload Folder found. Test files present.\n");
    }
}

// Function to display the number of clients connected
void display_connected_clients(int num_clients) {
    printf("Number of clients connected: %d\n", num_clients);
}

// Function to handle option 1: Get and display student info
void handle_student_info(int connfd, struct sockaddr_in client_addr) {
    printf("Request for Menu Option 1 received. Sending student information to client...\n");
    char student_id[] = "S2110913";
    char student_name[] = "Caroline Vivian Anyango Otuoma";
    char server_ip[INET_ADDRSTRLEN];
    char concatenated_string[256];

    inet_ntop(AF_INET, &(client_addr.sin_addr), server_ip, INET_ADDRSTRLEN);

    snprintf(concatenated_string, sizeof(concatenated_string), "Server IP: %s\nStudent ID: %s\nStudent Name: %s\n", server_ip, student_id, student_name);

    size_t len = strlen(concatenated_string) + 1;
    writen(connfd, (unsigned char *)&len, sizeof(size_t));
    writen(connfd, (unsigned char *)concatenated_string, len);
}

// Function to handle option 2: Get and display an array of 5 random numbers
void handle_random_numbers(int connfd) {
    printf("Request for Menu Option 2 received. Sending random numbers to client...\n");
    // Obtain 5 random numbers in the range 100 to 500
    int i;
    int random_numbers[5];

    for (i = 0; i < 5; ++i) {
        random_numbers[i] = rand() % 401 + 100; // Generates a random number between 100 and 500
    }

    // Send the array of random numbers to the client
    size_t payload_length = sizeof(random_numbers);
    writen(connfd, (unsigned char *)&payload_length, sizeof(size_t));
    writen(connfd, (unsigned char *)random_numbers, payload_length);
}

// Function to handle option 3: Get and display uname information
void handle_uname_info(int connfd) {
    printf("Request for Menu Option 3 received. Sending server uname information to client...\n");
    struct utsname server_info;
    if (uname(&server_info) == -1) {
        perror("Error getting uname information");
        return;
    }

    size_t payload_length = sizeof(struct utsname);

    // Send the utsname structure to the client
    writen(connfd, (unsigned char *)&payload_length, sizeof(size_t));
    writen(connfd, (unsigned char *)&server_info, payload_length);
}

// Function to handle option 4: Get and display list of files and directories in 'upload' directory
void handle_file_list(int connfd) {
    printf("Request for Menu Option 4 received. Sending file list to client...\n");
    char file_list[4096]; // Adjust the buffer size accordingly
    memset(file_list, 0, sizeof(file_list)); // Initialize buffer

    // Use scandir to get file and directory names in the 'upload' directory
    struct dirent **namelist;
    int n = scandir("upload", &namelist, NULL, alphasort);

    if (n == -1) {
        perror("Error reading directory");
        return;
    }

    // Iterate through the directory entries and construct the file list string
    int i;
        for (i = 0; i < n; ++i) {
        char path[256]; // Adjust the buffer size accordingly
        snprintf(path, sizeof(path), "upload/%s", namelist[i]->d_name);

        struct stat file_stat;
        if (stat(path, &file_stat) == -1) {
            perror("Error getting file information");
            continue;
        }

        // Check if the entry is a regular file or directory
        if (S_ISREG(file_stat.st_mode) || S_ISDIR(file_stat.st_mode)) {
            strcat(file_list, namelist[i]->d_name);
            strcat(file_list, "*"); // Use '*' as the separator
        }

        free(namelist[i]);
    }

    free(namelist);

    size_t len = strlen(file_list) + 1;
    writen(connfd, (unsigned char *)&len, sizeof(size_t));
    writen(connfd, (unsigned char *)file_list, len);
}


// Implementation of the handle_file_request function
void handle_file_request(int connfd) {
    printf("Request for Menu Option 5 received. Handling file request...\n");
    size_t payload_length;
    char filename[256];

    // Receive the file name from the client
    if (readn(connfd, &payload_length, sizeof(size_t)) <= 0) {
        perror("Error reading filename length from socket");
        return;
    }

    // Ensure payload_length is within bounds
    if (payload_length > sizeof(filename)) {
        fprintf(stderr, "Error: Received filename length exceeds buffer size\n");
        return;
    }

    if (readn(connfd, filename, payload_length) <= 0) {
        perror("Error reading filename from socket");
        return;
    }

    // Null-terminate the filename
    filename[payload_length] = '\0';

    printf("Received request for file: %s\n", filename);

    // Construct the full file path
    char filepath[256]; // Adjust the buffer size accordingly
    snprintf(filepath, sizeof(filepath), "upload/%s", filename);

    int filefd = open(filepath, O_RDONLY);
    if (filefd == -1) {
        perror("Error opening file");
        // Inform the client that the file does not exist
        size_t zero_size = 0;
        writen(connfd, &zero_size, sizeof(size_t));
        return;
    }

    struct stat file_stat;
    if (fstat(filefd, &file_stat) == -1) {
        perror("Error getting file information");
        close(filefd);
        return;
    }

    size_t file_size = file_stat.st_size;

    // Inform the client that the file exists
    size_t non_zero_size = 1;
    writen(connfd, &non_zero_size, sizeof(size_t));

    // Send the file size to the client
    if (writen(connfd, &file_size, sizeof(size_t)) <= 0) {
        perror("Error writing file size to socket");
        close(filefd);
        return;
    }

    // Send the file contents to the client
    char buffer[1024]; // Adjust the buffer size accordingly
    ssize_t bytes_read;
    while ((bytes_read = read(filefd, buffer, sizeof(buffer))) > 0) {
        if (writen(connfd, buffer, bytes_read) <= 0) {
            perror("Error writing file content to socket");
            close(filefd);
            return;
        }
    }
    // Send completion indication after the loop
    size_t zero_size = 0;
    writen(connfd, &zero_size, sizeof(size_t));

    printf("File '%s' sent successfully\n", filename);

    close(filefd);
}


int listenfd = 0; // Global variable to store the listening socket

// Function to handle server cleanup
void cleanup(int listenfd) {
    printf("Cleaning up...\n");

    // Close the listening socket
    if (close(listenfd) == -1) {
        perror("Error closing listening socket");
    }

    printf("Server cleanup complete\n");
}


// Global variable to store the start time of the server
struct timeval start_time;

// Signal handler for <ctrl> + c or SIGINT
void sigint_handler(int signo) {
    if (signo == SIGINT) {
        struct timeval end_time;
        gettimeofday(&end_time, NULL);

        // Calculate and display the total server execution time
        long seconds = end_time.tv_sec - start_time.tv_sec;
        long microseconds = end_time.tv_usec - start_time.tv_usec;
        double total_time = seconds + microseconds * 1e-6;

        printf("\nServer received SIGINT. Exiting gracefully...\n");
        printf("Total Server Execution Time: %.6f seconds\n", total_time);

        // Cleanup and exit
        // Add any additional cleanup steps here if needed
        exit(EXIT_SUCCESS);
    }
}


// you shouldn't need to change main() in the server except the port number
int main(void)
{
    int connfd = 0;
    int num_clients = 0; // Variable to keep track of connected clients

    // Register the signal handler for SIGINT
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Error registering signal handler");
        exit(EXIT_FAILURE);
    }

    // Get the start time of the server
    gettimeofday(&start_time, NULL);

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50001);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
	perror("Failed to listen");
	exit(EXIT_FAILURE);
    }
    // end socket setup

    // Ensure the "upload" directory exists and create initial files
    ensure_upload_directory();

    // Display the number of connected clients
    display_connected_clients(num_clients);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");

        // Increment the number of connected clients
        num_clients++;

        // Display the number of connected clients
        display_connected_clients(num_clients);

	pthread_t sniffer_thread;
        // third parameter is a pointer to the thread function, fourth is its actual parameter
	int *connfd_ptr = (int *)malloc(sizeof(int));
	if (connfd_ptr == NULL) {
	    perror("Error allocating memory for connfd");
	    exit(EXIT_FAILURE);
	}

	*connfd_ptr = connfd;

	if (pthread_create(&sniffer_thread, NULL, client_handler, (void *)connfd_ptr) < 0) {
	    perror("could not create thread");
	    free(connfd_ptr);  // Free the allocated memory
	    exit(EXIT_FAILURE);
	}

	//Now join the thread , so that we dont terminate before the thread
	//pthread_join( sniffer_thread , NULL);
	printf("Handler assigned\n");
    }

    // never reached...
    // ** should include a signal handler to clean up
    exit(EXIT_SUCCESS);
} // end main()

// thread function - one instance of each for each connected client
// this is where the do-while loop will go
void *client_handler(void *socket_desc)
{
    // Get the socket descriptor
    int connfd = *(int *) socket_desc;
    free(socket_desc);  // Free the memory allocated in the main function
    struct sockaddr_in client_addr;

    socklen_t socksize = sizeof(struct sockaddr_in);
    getpeername(connfd, (struct sockaddr *)&client_addr, &socksize);

    int option;

    do {
        // Get user option
        readn(connfd, (unsigned char *)&option, sizeof(int));

        // Handle user option
        switch (option) {
            case 1:
                handle_student_info(connfd, client_addr);
                break;
            case 2:
                handle_random_numbers(connfd);
                break;
            case 3:
                handle_uname_info(connfd);
                break;
            case 4:
                handle_file_list(connfd);
                break;
            case 5:
		printf("Waiting for Client to enter file name...\n");
                handle_file_request(connfd);
                break;
            case 6:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid option\n");
                break;
        }
    } while (option != 6);

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    // always clean up sockets gracefully
    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
} // end client_handler()
