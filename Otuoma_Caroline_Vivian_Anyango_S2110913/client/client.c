#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <stdbool.h>
#include "rdwrn.h"


void receiveFile(int socket, const char *filename);

void get_student_info(int socket) {
    printf("Receiving student information from the server...\n");
    char received_string[256];
    size_t k;

    if (readn(socket, (unsigned char*)&k, sizeof(size_t)) < 0) {
        perror("Error reading length of student info");
        return;
    }

    if (readn(socket, (unsigned char *)received_string, k) < 0) {
        perror("Error reading student info");
        return;
    }

    printf("Received Student Info from the server:\n%s\n", received_string);
}

void get_random_numbers(int socket) {
    printf("Receiving random numbers from the server...\n");
    int i;
    int received_numbers[5];
    size_t payload_length;

    if (readn(socket, (unsigned char *)&payload_length, sizeof(size_t)) < 0) {
        perror("Error reading length of random numbers");
        return;
    }

    if (readn(socket, (unsigned char *)received_numbers, payload_length) < 0) {
        perror("Error reading random numbers");
        return;
    }

    printf("Received Random Numbers from the server:\n");
    for (i = 0; i < 5; ++i) {
        printf("%d\n", received_numbers[i]);
    }
    printf("\n");

    int sum = 0;
    for (i = 0; i < 5; ++i) {
        sum += received_numbers[i];
    }
    printf("Sum of Random Numbers: %d\n", sum);
}

void get_system_info(int socket) {
    printf("Receiving server uname information from the server...\n");

    size_t payload_length;
    struct utsname server_info;

    readn(socket, (unsigned char *)&payload_length, sizeof(size_t));
    readn(socket, (unsigned char *)&server_info, payload_length);

    printf("\n-----------------------------------\n");
    printf("SERVER INFORMATION:\n");
    printf("-----------------------------------\n");
    printf("OS Name: %s\n", server_info.sysname);
    printf("Release: %s\n", server_info.release);
    printf("Version: %s\n", server_info.version);
    printf("\n");
}

void getList(int socket) {
    printf("Receiving file list from the server...\n");
    size_t payload_length;

    if (readn(socket, (unsigned char *)&payload_length, sizeof(size_t)) < 0) {
        perror("Error reading length of file list");
        return;
    }

    char *file_list = (char *)malloc(payload_length);
    if (file_list == NULL) {
        perror("Error allocating memory for file list");
        return;
    }

    if (readn(socket, (unsigned char *)file_list, payload_length) < 0) {
        perror("Error reading file list");
        free(file_list);
        return;
    }

    printf("File List:\n");
    char *token = strtok(file_list, "*");
    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, "*");
    }

    free(file_list);
}

void receiveFile(int socket, const char *filename) {
    printf("Receiving file '%s' from the server...\n", filename);
    size_t file_size;

    if (readn(socket, &file_size, sizeof(size_t)) <= 0) {
        perror("Error reading file size from socket");
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    char buffer[1024];
    size_t bytes_received = 0;
    ssize_t bytes_read;

    while (bytes_received < file_size) {
        size_t remaining_bytes = file_size - bytes_received;
        size_t to_read = remaining_bytes > sizeof(buffer) ? sizeof(buffer) : remaining_bytes;

        bytes_read = readn(socket, buffer, to_read);
        if (bytes_read <= 0) {
            perror("Error reading file content from socket");
            fclose(file);
            return;
        }

        size_t bytes_written = fwrite(buffer, 1, bytes_read, file);
        if (bytes_written != bytes_read) {
            perror("Error writing file content");
            fclose(file);
            return;
        }

        bytes_received += bytes_read;
    }

    printf("File received: %s\n", filename);

    size_t completion_indicator;
    if (readn(socket, &completion_indicator, sizeof(size_t)) <= 0) {
        perror("Error reading completion indication from socket");
        fclose(file);
        return;
    }

    fclose(file);
}

bool fileExists(const char *filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "upload/%s", filename);

    return access(filepath, F_OK) != -1;
}


void display_menu() {
    printf("\n-----------------------------------------------------------\n");
    printf("           MENU\n");
    printf("-----------------------------------------------------------\n");
    printf("1. Display name and student ID\n");
    printf("2. Display 5 random numbers and compute sum\n");
    printf("3. Display server uname information\n");
    printf("4. Display list of files in 'upload' directory\n");
    printf("5. Request a file from the server\n");
    printf("6. Exit\n");
    printf("-----------------------------------------------------------\n");
}

int main(void)
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Error - could not create socket");
	exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(50001);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	perror("Error - connect failed");
	exit(1);
    } else
       printf("Connected to server...\n");

	int option;
    do {
        display_menu();
        printf("Enter your choice (1-6): ");
        scanf("%d", &option);

        writen(sockfd, (unsigned char *)&option, sizeof(int));

        switch (option) {
            case 1:
    		get_student_info(sockfd);
                break;
            case 2:
                get_random_numbers(sockfd);
                break;
            case 3:
                get_system_info(sockfd);
                break;
            case 4:
                getList(sockfd);
                break;
	    case 5: {
	        printf("Option 5: Requesting file from server side...\n");
	        char file_from_server[256];

	        printf("Enter the filename to copy: ");
	        if (scanf("%255s", file_from_server) != 1) {
			perror("Error reading filename from stdin");
		break;
	    	}

	    size_t filename_length = strlen(file_from_server) + 1;


	    if (writen(sockfd, &filename_length, sizeof(size_t)) <= 0) {
	        perror("Error writing filename length to socket");
	        break;
	    }

	    if (writen(sockfd, file_from_server, filename_length) <= 0) {
	        perror("Error writing filename to socket");
	        break;
	    }

	    bool file_exists_on_server;
	    if (readn(sockfd, &file_exists_on_server, sizeof(bool)) <= 0) {
    		perror("Error reading file existence status from socket");
 		break;
	    }


	    if (!file_exists_on_server) {
	   	 printf("Error: File '%s' does not exist on the server. Try again.\n", file_from_server);
	   	 break;
	    }

	    size_t file_size;
	    if (readn(sockfd, &file_size, sizeof(size_t)) <= 0) {
		perror("Error reading file size from socket");
		break;
	    }
	    receiveFile(sockfd, file_from_server);

	  	  break;
	    }
            case 6:
                printf("Exiting the client...\n");
                break;
            default:
                printf("Invalid option. Please enter a valid option.\n");
        }
    } while (option != 6);

    close(sockfd);

    exit(EXIT_SUCCESS);
}
