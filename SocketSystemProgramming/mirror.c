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

#define SIZE 1024 //defaulting string size


char *ltrim(char *s) //function to trim left spaces
{     
    while(isspace(*s)) s++;     
    return s; 
}  

char *rtrim(char *s) //function to trim right spaces
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

char *trim(char *s) //function to trim both sides
{     
    return rtrim(ltrim(s));  
} 

int processclient(int new_sock){ //function to process client request
    char opt[SIZE]; //to store output
    char word[SIZE]; //to store input
    char command[SIZE]; //to store command
    char var[SIZE]; //to store variable
    char var1[SIZE]; //to store variable1
    char var2[SIZE]; //to store variable2
    
    while(1){ //infinite loop to keep server running
        bzero(word, SIZE); //clearing output after every iteration
        bzero(opt, SIZE); //clearing output after every iteration
        bzero(command, SIZE); //clearing output after every iteration

        if (read(new_sock, word, sizeof(word))<0){ //reading input from client
            printf("read() error\n"); //error message
            exit(3); 
        }
        
        printf("\nMessage received from client: %s \n", word); //printing input from client
        if (strncmp(word, "findfile", 8) == 0){ //checking if input is findfile
            memmove(word, word+9, strlen(word)-9+1); //removing findfile from input
            strcpy(command, "find ~ -name ");
            strcat(command, trim(word));
            strcat(command, " -printf '\%f \%s \%TD\n' | head -n 1"); //creating command to find file      

            FILE *f = popen(command, "r"); //executing command
            while(fgets(opt, SIZE, f)!= NULL) { //reading output of command
                if (send(new_sock, opt, sizeof(opt), 0) == -1) { //sending output to client
                    perror("[-]Error in sending message.");
                    exit(1);
                }
            }        
            if(opt[0] == '\0') { //if file not found
                strcpy(opt, "File Not Found\n"); //sending error message to client        
                if (send(new_sock, opt, sizeof(opt), 0) == -1) { //sending error message to client
                    perror("[-]Error in sending message.");
                    exit(1);
                }            
            }
        
        }
        else if(strncmp(word, "sgetfiles", 9) == 0){ //checking if input is sgetfiles
            char var1[SIZE];
            char var2[SIZE];

            memmove(word, word+10, strlen(word)-10+1); //removing sgetfiles from input
            sscanf(word, "%s %s", var1, var2); //splitting input into two variables
            strcpy(command, "find ~ -type f -size +");
            strcat(command, trim(var1));
            strcat(command,"c -size -");
            strcat(command, trim(var2));
            strcat(command, "c -print0 -quit | grep -q . && find ~ -type f -size +");
            strcat(command, trim(var1));
            strcat(command, "c -size -");
            strcat(command, trim(var2));
            strcat(command, "c -print0 | tar -cf temp.tar.gz --null -T - 2>/dev/null"); //creating command to find files by size
            
            system(command); //executing command
            sleep(1);
            FILE *f = fopen("temp.tar.gz", "r"); //opening file
            if (f == NULL) { //if file not found
                strcpy(opt, "ERROR");        
                if (send(new_sock, opt, sizeof(opt), 0) == -1) { //sending error message to client
                    perror("[-]Error in sending message.");
                    exit(1);
                }
            }else{
                int b; //to store size of file
                while( (b = fread(opt, 1, sizeof(opt), f))>0 ){ //reading file
                    send(new_sock, opt, b, 0); //sending file to client
                }
                printf("\nFile sent\n");
                strcpy(opt, "END"); //sending end message to client
                send(new_sock, opt, sizeof(opt), 0); //sending end message to client

                fclose(f); //closing file
                sleep(1);
                system("rm temp.tar.gz"); //removing file from server after sending to client

            // }
            }
        }
        else if(strncmp(word, "dgetfiles", 9) == 0){ //checking if input is dgetfiles
            char var1[SIZE];
            char var2[SIZE];

            memmove(word, word+10, strlen(word)-10+1); //removing dgetfiles from input
            sscanf(word, "%s %s", var1, var2); //splitting input into two variables
            strcpy(command, "find ~ -type f -newermt ");
            strcat(command, trim(var1));
            strcat(command," ! -newermt ");
            strcat(command, trim(var2));
            strcat(command, " -print0 | grep -q . && find ~ -type f -newermt ");
            strcat(command, trim(var1));
            strcat(command, " ! -newermt ");
            strcat(command, trim(var2));
            strcat(command, " -print0 | tar -cf temp.tar.gz --null -T - 2>/dev/null"); //creating command to find files by date

            system(command); //executing command
            sleep(1);
            FILE *f = fopen("temp.tar.gz", "r"); //opening file
            if (f == NULL) { //if file not found
                strcpy(opt, "ERROR");        
                if (send(new_sock, opt, sizeof(opt), 0) == -1) { //sending error message to client
                    perror("[-]Error in sending message.");
                    exit(1);
                }
            }else{
                int b; //to store size of file
                while( (b = fread(opt, 1, sizeof(opt), f))>0 ){ //reading file
                    send(new_sock, opt, b, 0); //sending file to client
                }
                printf("\nFile sent\n");
                strcpy(opt, "END"); //sending end message to client
                send(new_sock, opt, sizeof(opt), 0); //sending end message to client

                fclose(f); //closing file
                sleep(1);
                system("rm temp.tar.gz"); //removing file from server after sending to client

            // }
            }
        }
        else if(strncmp(word, "getfiles", 8) == 0){ //checking if input is getfiles
            strcpy(command, "find ~ -type f \\( -name"); //creating command to find files by name
            // tokenizing the input string to get the file names separated by spaces to be searched
            char* token = strtok(word, " "); //tokenizing input
            char var[SIZE];
            strcpy(var, " ");
            while (token != NULL) { //looping through tokens
                if (strcmp(token, "-u") != 0 && strcmp(token, "getfiles") != 0 ) {
                    strcat(command, var);
                    strcat(command, " ");
                    strcat(command, trim(token));
                    strcpy(var, " -o -name");
                }
                
                token = strtok(NULL, " ");
            }
            strcat(command, " \\) -exec tar -cf temp.tar.gz {} + 2>/dev/null"); //creating command to find files by name
            system(command); //executing command
            sleep(1);
            FILE *f = fopen("temp.tar.gz", "r"); //opening file
            if (f == NULL) { //if file not found
                strcpy(opt, "ERROR");        
                if (send(new_sock, opt, sizeof(opt), 0) == -1) { //sending error message to client
                    perror("[-]Error in sending message.");
                    exit(1);
                }
            }else{
                int b; //to store size of file
                while( (b = fread(opt, 1, sizeof(opt), f))>0 ){ //reading file
                    send(new_sock, opt, b, 0); //sending file to client
                }
                printf("\nFile sent\n");
                strcpy(opt, "END"); //sending end message to client
                send(new_sock, opt, sizeof(opt), 0); //sending end message to client

                fclose(f); //closing file
                sleep(1);
                system("rm temp.tar.gz"); //removing file from server after sending to client

            // }
            }
        }
        else if(strncmp(word, "gettargz", 8) == 0){ //checking if input is gettargz
            // tokenizing the input string to get the file names separated by spaces to be searched
            strcpy(command, "find ~ -type f \\( -name \"*.");
            char* token = strtok(word, " ");
            char var[SIZE];
            strcpy(var, "");
            while (token != NULL) {
                if (strcmp(token, "-u") != 0 && strcmp(token, "gettargz") != 0 ) { //checking if token is not -u or gettargz
                    strcat(command, var);
                    strcat(command, trim(token));
                    strcpy(var, "\" -o -name \"*."); //creating command to find files by extension
                }
                
                token = strtok(NULL, " ");
            }
            strcat(command, "\" \\) -exec tar -cf temp.tar.gz {} + 2>/dev/null"); //creating command to find files by extension
            system(command); //executing command
            sleep(1);
            FILE *f = fopen("temp.tar.gz", "r"); //opening file
            if (f == NULL) { //if file not found
                strcpy(opt, "ERROR");        
                if (send(new_sock, opt, sizeof(opt), 0) == -1) { //sending error message to client
                    perror("[-]Error in sending message.");
                    exit(1);
                }
            }else{

                int b; //to store size of file
                while( (b = fread(opt, 1, sizeof(opt), f))>0 ){ //reading file
                    send(new_sock, opt, b, 0); //sending file to client
                }
                printf("\nFile sent\n");
                strcpy(opt, "END"); //sending end message to client
                send(new_sock, opt, sizeof(opt), 0); //sending end message to client

                fclose(f); //closing file
                sleep(1);
                system("rm temp.tar.gz"); //removing file from server after sending to client

            // }
            }
        }
        else if(strcmp(word, "quit") == 0) //if passed string is quit close the socket and kill child process
        {
            printf("\nClient Disconnected\n"); //printing client disconnected message
            close(new_sock); //closing socket
            exit(0); //killing child process
        }       
    }
}

int main(){
    char *ip = "127.0.0.1"; //setting ip address for server
    char *ip1 = "127.0.0.1"; //setting ip address for mirror
    int portA = htons(8080); //setting port for server
    int portB = htons(8081); //setting port for mirror
    int eA, eB, count = 0; //initializing eA and eB to store binding output & count for load balancing
    int flag = 0; // flag to select which server or mirror to connect to
    int sockfdA, sockfdB, new_sock; //socket for server and mirror along with file descriptor of connected socket
    struct sockaddr_in server_addr, new_addr; //connection struct 
    socklen_t addr_size; //address length

    sockfdA = socket(AF_INET, SOCK_STREAM, 0); //socket for server
    sockfdB = socket(AF_INET, SOCK_STREAM, 0); //socket for mirror
    if(sockfdA < 0) { //error in socket initialization
        perror("Error in socketA");
        exit(1);
    }
    if(sockfdA < 0) { //error in mirror initialization
        perror("Error in socketB");
        exit(1);
    }
    printf("Server Socket Created Successfully.\n"); //printing socket creation success message
    //setting connection params
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = portA;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    //binding server socket to port
    eA = bind(sockfdA, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // If binding fails, then port is occupied by server, try to bind as mirror
    if(eA < 0) {
        //change ip and port as per mirror
        server_addr.sin_port = portB;
        server_addr.sin_addr.s_addr = inet_addr(ip1);
        flag = 1;
        eB = bind(sockfdB, (struct sockaddr*)&server_addr, sizeof(server_addr));
        printf("Binding successful with Mirror\n");
        if(listen(sockfdB, 5) == 0){
        }   
        else{
            perror("Error in listening B");
            exit(1);
        }
    }
    else{
        printf("Binding successful with Server\n");
        
    }
    //setting up socket listening capacity
    if(listen(sockfdA, 200) == 0){
        }   
    else{
        perror("Error in listening A");
        exit(1);
    }
    //setting up mirror listening capacity
    if(listen(sockfdB, 200) == 0){
    }   
    else{
        perror("Error in listening B");
        exit(1);
    }
    addr_size = sizeof(new_addr);

    if (flag==0){ //flag 0 indicates always connect to server and 1 indicates always connect to mirror 
        while(1) //infinite loop to keep server running
        {   
            new_sock = accept(sockfdA, (struct sockaddr*)&new_addr, &addr_size); //accpet the connection
            if(count>=0 && count<4) { //if current count is between 0 to 4 connect to server
                count++; //to count number of clients connected
                send(new_sock, "Welcome", sizeof("Welcome"), 0); //send welcome message(positive acknowledgement)
            }
            else if(count>=4 && count<8){//if current count is between 5 to 9 connect to mirror
                count++; //to count number of clients connected
                send(new_sock, "NA", sizeof("NA"), 0); //send NA negative acknowledgement
                sleep(1);
                close(new_sock); //close the connection
                printf("\nTransferred to mirror\n"); //print message to show transfer to mirror
                continue; //keep looking for new clients
            }
            else if(count % 2 == 0){//even number of client connection after 8(eg 10,12,14, ...)
                count++; //to count number of clients connected
                send(new_sock, "Welcome", sizeof("Welcome"), 0); //send welcome message(positive acknowledgement)
            }
            else{ //odd number of client connection after 8 (eg 9,11,13,15, ...)
                count++; //to count number of clients connected
                send(new_sock, "NA", sizeof("NA"), 0); //send NA(negative acknowledgement)
                sleep(1);
                close(new_sock);  //close the connection
                printf("Transferred to mirror\n\n"); //print message to show transfer to mirror
                continue; //keep looking for new clients
            }
            printf("\nReceived a Client\n"); //print message to show new client connected
            if(!fork()){ //let child process handle the client and parent goes back to look for new client
                processclient(new_sock); //call processclient function to handle client
            }
            close(new_sock); //close the socket in parent process
        }  
        }
    else{
        // Code for mirror, Dumb server - accept whatever comes
        while(1){ //infinite loop to keep server running
            new_sock = accept(sockfdB, (struct sockaddr*)&new_addr, &addr_size);
            send(new_sock, "Welcome", sizeof("Welcome"), 0); //send welcome message(positive acknowledgement)
            printf("\nReceived a Client\n");
            if(!fork()){ //let child process handle the client and parent goes back to look for new client
                processclient(new_sock); //call processclient function to handle client
            }
            close(new_sock); //close the socket in parent process
        }
    }
}