#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#define SIZE 1024 // Size of buffer

char *ltrim(char *s) //function to remove leading spaces
{     
    while(isspace(*s)) s++;     
    return s; 
}  

char *rtrim(char *s) //function to remove trailing spaces
{     
    char* back;
    int len = strlen(s);

    if(len == 0)
        return(s); 

    back = s + len;     
    while(isspace(*--back));     
    *(back+1) = '\0';     
    return s; 
}  

char *trim(char *s) //function to remove leading and trailing spaces
{     
    return rtrim(ltrim(s));  
} 

// Use Colours for bash text
#define SET_COLOUR    "\x1b[33m" // Yellow
#define RESET_COLOR   "\x1b[0m" // Reset

void write_file(int sockfd){ //function to read file from server
    int n;
    FILE *fp; //file pointer
    char buffer[SIZE]; //buffer to store data
    
    fp = fopen("temp.tar.gz", "w+"); //open file in write mode

    int total = 0; //to store total bytes received
    while (1){ //loop to receive data from server
        n = recv(sockfd, buffer, SIZE, 0); //receive data from server
        if (strcmp(buffer, "END") == 0) //if end of file is reached break
            break;
        else if(strcmp(buffer, "ERROR") == 0){ //if file not found print error and break
            printf("File not found\n"); //print error
            fclose(fp); //close file
            system("rm temp.tar.gz 2>/dev/null"); //remove file from client
            return;
        }
        total += n; //increment total bytes received
        fwrite(buffer, sizeof(buffer), 1, fp); //write data to file
        bzero(buffer, SIZE); //clear buffer
    }
    printf("Total bytes received: %d \n", total); //print total bytes received
    fclose(fp); //close file

    printf("File received successfully.\n"); //print success message
    return;
}


int main(int argc, char *argv[]){ //main function
    // Change ip if running on two different devices and replace with device ip
    char *ip1 = "127.0.0.1";
    char *ip = "127.0.0.1";
    // Server running on port 8080 and mirror on port 8081
    int port = htons(8080);
    int port2 = htons(8081);
    int e;
    // Define descriptor for socket
    int sockfd;
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("Error in socket");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));  //clear pervious server memory
    //setting up address for server
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    //connect to server
    e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e == -1) { //if we get error while initializing socket
        perror("Error in socket");
        exit(1);
    }
	printf("Connected to Server.\n"); //print success message
    
    int n; //to receive the length of input message
    char input[SIZE]; //to get input value
    char buffer[SIZE]; //to get what acknowledgement from server  
    
    n = recv(sockfd, buffer, sizeof(buffer), 0); //receive the acknowledgement from server in buffer
    if (strncmp(buffer, "NA", 2) == 0){ //incase of negative ack connect to mirror
        close(sockfd); //close previous socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0); //creating mirror socket
        struct sockaddr_in server_addr2; //to store connection address for mirror
        memset(&server_addr2, 0, sizeof(server_addr)); //clear pervious server memory
        //setting up address for mirror
        server_addr2.sin_family = AF_INET; 
        server_addr2.sin_port = port2;
        server_addr2.sin_addr.s_addr = inet_addr(ip1);
        e = connect(sockfd, (struct sockaddr*)&server_addr2, sizeof(server_addr2)); //connect to mirror
        if(e == -1) { //if we get error while initializing socket
            perror("Error in socket");
            exit(1);
        }
    recv(sockfd, buffer, sizeof(buffer), 0); //receive the message welcome from mirror in buffer variable
    }
    while (1) {

        memset(buffer, 0, SIZE); // Clear value of variable
        memset(input, 0, SIZE); // Clear value of variable
        printf(SET_COLOUR "C$ " RESET_COLOR); // Print prompt
        fgets(input, sizeof(input), stdin); // Get input
        strcpy(input, trim(input)); //remove trailing and leading spaces
        
        if (strncmp(input, "findfile", 8) == 0){ //if input is findfile
            int count = 0; //to count number of arguments
            char temp[SIZE]; //to store input
            strcpy(temp, input); //copy input to temp
            int len = strlen(input); //get length of input
            char *token = strtok(input, " "); //split input by space
            while (token != NULL) { //loop to count number of arguments
                count++;
                token = strtok(NULL, " ");
            }
            // recv(sockfd, buffer, sizeof(buffer), 0);
            //printf("count: %d\n", count);
            if (count == 2){ //if number of arguments is 2
                write(sockfd, temp, sizeof(temp)); // Write to server
                sleep(1);      
                recv(sockfd, buffer, sizeof(buffer), 0); //get response from server in normal cases
                printf("%s", buffer); //print the response
            }else{ //if number of arguments is not 2
                printf("Error: Invalid number of arguments\n"); //print error
            }
        
        }else if(strncmp(input, "sgetfiles", 9) == 0){ //if input is sgetfiles
            int count = 0; //to count number of arguments
            char temp[SIZE]; //to store input
            strcpy(temp, input); //copy input to temp
            int len = strlen(input); //get length of input
            char *token = strtok(input, " "); //split input by space
            while (token != NULL) { //loop to count number of arguments
                count++;
                token = strtok(NULL, " "); //split input by space
            }
            if (len >= 3 && temp[len-1] == 'u' && temp[len-2] == '-'){ //if last two characters are -u
                count -= 1; //decrement count by 1
            }
            if (count == 3){ //if number of arguments is 3
                write(sockfd, temp, sizeof(temp)); // Write to server
                sleep(1);
                write_file(sockfd); //call write_file function to write file to receive the file sent from the server
                int len = strlen(temp); //get length of input
                if (len >= 3 && temp[len-1] == 'u' && temp[len-2] == '-') { //if last two characters are -u
                    system("tar -xf temp.tar.gz"); //unzip the file
                    sleep(1);
                    system("rm temp.tar.gz 2> /dev/null"); //remove the file
                    // printf("File unzipped successfully.\n");
                }
            }else{
                printf("Error: Invalid number of arguments\n"); //print error
            }

        }else if(strncmp(input, "dgetfiles", 9) == 0){ //if input is dgetfiles
            int count = 0; //to count number of arguments
            char temp[SIZE]; //to store input
            strcpy(temp, input); //copy input to temp
            int len = strlen(input); //get length of input
            char *token = strtok(input, " "); //split input by space
            while (token != NULL) { //loop to count number of arguments
                count++;
                token = strtok(NULL, " ");
            }
            if (len >= 3 && temp[len-1] == 'u' && temp[len-2] == '-') //if last two characters are -u
                count -= 1;
            // printf("count: %d\n", count);
            if (count == 3){
                write(sockfd, temp, sizeof(temp)); // Write to server
                sleep(1);
                write_file(sockfd); //call write_file function to write file
                int len = strlen(temp); //get length of input
                if (len >= 3 && temp[len-1] == 'u' && temp[len-2] == '-') { //if last two characters are -u
                    system("tar -xf temp.tar.gz 2> /dev/null"); //unzip the file
                    // file output comes in the form of / to suppress warnings we have used 
                    sleep(1);
                    system("rm temp.tar.gz 2> /dev/null"); //remove the file
                }
            }else{
                printf("Error: Invalid number of arguments\n"); //print error
            }
        
        }else if(strncmp(input, "getfiles", 8) == 0){ //if input is getfiles
            int count = 0; //to count number of arguments
            char temp[SIZE]; //to store input
            strcpy(temp, input); //copy input to temp
            int len = strlen(input); //get length of input
            char *token = strtok(input, " "); //split input by space
            while (token != NULL) { //loop to count number of arguments
                count++;
                token = strtok(NULL, " ");
            }

            /*
            The next line calls strtok again with NULL as the first argument and " " as the second argument, 
            which continues tokenizing the original string starting from where it left off in the previous 
            call to strtok. 
            The pointer to the next token is then assigned back to the token variable.
            */

            if (len >= 3 && input[len-1] == 'u' && input[len-2] == '-') //if last two characters are -u
                count -= 1; //decrement count by 1
            if (count > 1 && count < 8){ //if number of arguments is between 2 and 7
                write(sockfd, temp, sizeof(temp)); // Write to server
                sleep(1);
                write_file(sockfd); //call write_file function to write file
    
                if (len >= 3 && input[len-1] == 'u' && input[len-2] == '-') { //if last two characters are -u
                    system("tar -xf temp.tar.gz 2> /dev/null"); //unzip the file
                    sleep(1);
                    system("rm temp.tar.gz 2>/dev/null"); //remove the file
                    // printf("File unzipped successfully.\n");
                }
            }else{
                printf("Error: Invalid number of arguments\n"); //print error
            }

        }else if(strncmp(input, "gettargz", 8) == 0){ //if input is gettargz
            int count = 0; //to count number of arguments
            char temp[SIZE]; //to store input
            strcpy(temp, input); //copy input to temp
            int len = strlen(input); //get length of input
            char *token = strtok(input, " "); //split input by space
            while (token != NULL) {
                count++; //loop to count number of arguments
                token = strtok(NULL, " "); //split input by space
            }
            if (len >= 3 && input[len-1] == 'u' && input[len-2] == '-') //if last two characters are -u
                count -= 1; //decrement count by 1
            if (count > 1 && count < 8){ //if number of arguments is between 2 and 7
                write(sockfd, temp, sizeof(temp)); // Write to server
                sleep(1);
                write_file(sockfd); //call write_file function to write file
    
                if (len >= 3 && input[len-1] == 'u' && input[len-2] == '-') { //if last two characters are -u
                    system("tar -xf temp.tar.gz 2> /dev/null"); //unzip the file
                    sleep(1);
                    system("rm temp.tar.gz 2>/dev/null"); //remove the file
                    // printf("File unzipped successfully.\n");
                }
            }else{
                printf("Error: Invalid number of arguments\n"); //print error
            }

        }else if(strcmp(input, "quit") == 0) //if input is quit
        {
            write(sockfd, input, sizeof(input)); // Write to server
            sleep(1);
            close(sockfd); //close socket
            exit(0); //exit
        }
    }
    return 1;
}