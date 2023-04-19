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

// This will run for child process after forking
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


            // printf("findfile command is %s",command);

            /*
     This command uses the find utility to search for a file named mozilla.pdf in the user's home directory and prints specific information about the file if it is found. Here's an explanation of each part of the command:

find ~ -name mozilla.pdf:

find is a command-line utility to search for files and directories in a directory hierarchy.
~ specifies the user's home directory as the search starting point.
-name mozilla.pdf filters the search results to include only files with the name mozilla.pdf.
-printf '%f %s %TD':

-printf is an action in the find command that allows you to print specific information about the files found by the search using a format string.
The format string %f %s %TD consists of three placeholders separated by spaces:
%f: Replaced by the base name of the file (i.e., the file name without the directory path).
%s: Replaced by the file size in bytes.
%TD: Replaced by the last modification date of the file in the format MM/DD/YYYY.
In summary, this command searches the user's home directory for a file named mozilla.pdf and, if found, prints the file name, its size in bytes, and its last modification date in MM/DD/YYYY format, separated by spaces. 
            */




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
            
            // printf("sgetfiles command is %s",command);
            /*
            This command consists of two parts connected by &&. The first part checks whether there are any files within the specified size range in the user's home directory, and the second part creates a tarball (compressed archive) containing those files if they exist. Let's break down each part of the command:

First part: find ~ -type f -size +1240c -size -12450c -print0 -quit | grep -q .
This command searches for files in the user's home directory (~) that have a size between 1240 bytes and 12450 bytes, exclusive.

find ~ starts the search in the user's home directory.
-type f specifies that we're looking for files only (not directories).
-size +1240c and -size -12450c set the size range for the files, in bytes.
-print0 prints the results with a null character delimiter instead of a newline, allowing for filenames with spaces or other special characters.
-quit stops the search after the first match.
| grep -q . pipes the output to grep, which checks for any characters (represented by .). The -q flag suppresses any output and returns a success status if any matches are found.
Second part: find ~ -type f -size +1240c -size -12450c -print0 | tar -cf temp.tar.gz --null -T - 2>/dev/null
This part of the command is executed only if the first part returns a success status (i.e., if there are any matching files).

find ~ -type f -size +1240c -size -12450c -print0 repeats the search from the first part of the command, finding all files that match the specified criteria.
| tar -cf temp.tar.gz --null -T - pipes the output to tar to create a compressed archive of the matching files.
-cf temp.tar.gz specifies the output file temp.tar.gz and instructs tar to create a new archive.
--null tells tar to expect a null character delimiter in the input, which matches the -print0 flag in the find command.
-T - takes the list of files to be archived from standard input (-).
2>/dev/null redirects any error messages to /dev/null, essentially suppressing them.
In summary, this command checks whether there are any files within the specified size range in the user's home directory, and if so, creates a tarball containing those files.
            
            */




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


            // printf("dgetfiles command is %s",command);

            /*
            This is a shell command that utilizes various utilities to find and archive files with specific date constraints. Here's an explanation of each part of the command:

find ~ -type f -newermt 2017-01-01 ! -newermt 2023-04-11 -print0:

find is a command-line utility to search for files and directories in a directory hierarchy.
~ specifies the user's home directory as the search starting point.
-type f filters the search results to include only files.
-newermt 2017-01-01 includes files that are newer (modified after) the given date (2017-01-01).
! -newermt 2023-04-11 excludes files that are newer (modified after) the given date (2023-04-11).
-print0 prints the search results separated by a null character (to handle file names with spaces or special characters).
grep -q .:

grep is a command-line utility to search for specific patterns in text.
-q makes grep run in quiet mode, where it doesn't output the matched lines but returns an exit status indicating whether a match was found.
. is a regular expression that matches any single character. In this context, it is used to check if there is at least one file found by the find command.
&& is a shell control operator that only runs the following command if the previous command succeeds (returns an exit status of 0).

find ~ -type f -newermt 2017-01-01 ! -newermt 2023-04-11 -print0 | tar -cf temp.tar.gz --null -T - 2>/dev/null:

The find command here is the same as in step 1.
tar -cf temp.tar.gz --null -T - creates a tarball (compressed archive) of the files found by the find command.
-cf temp.tar.gz specifies the archive file to be created (temp.tar.gz).
--null tells tar to interpret the input file names as null-separated.
-T - reads the list of file names to be archived from standard input.
2>/dev/null redirects standard error (file descriptor 2) to /dev/null, effectively silencing any error messages from the tar command.
In summary, this command checks if there are any files in the user's home directory that were modified between 2017-01-01 and 2023-04-11 (inclusive). If there are such files, it creates a tarball (temp.tar.gz) containing those files.
            
            */

            system(command); //executing command
            sleep(1);
            FILE *f = fopen("temp.tar.gz", "r"); //opening file || to sent the file to client we have to first open the file
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
            
            // printf("getfiles command is %s",command);

            /*
            This is a shell command that uses the find utility to search for specific files in the user's home directory and archives them into a tarball (compressed archive) named temp.tar.gz. Here's an explanation of each part of the command:

find ~ -type f \( -name sample.txt -o -name SD.docx -o -name input.txt -o -name samplesong.mp3 \):

find is a command-line utility to search for files and directories in a directory hierarchy.
~ specifies the user's home directory as the search starting point.
-type f filters the search results to include only files.
\( and \) are used to group the expressions within, allowing for a more complex search pattern.
-name sample.txt, -name SD.docx, -name input.txt, and -name samplesong.mp3 specify the file names to search for. Each file name is connected with the -o operator, which means "or", allowing the search to include any of the specified file names.
-exec tar -cf temp.tar.gz {} +:

-exec is an action in the find command that allows you to execute a specific command for each file found by the search.
tar -cf temp.tar.gz {} creates a tarball (compressed archive) of the found files.
-cf temp.tar.gz specifies the archive file to be created (temp.tar.gz).
{} is a placeholder that gets replaced by the file name found by the find command for each matched file.
+ is used to indicate that the tar command should be called once with all the matched files as arguments, rather than once per matched file. This ensures that all matched files are added to the same archive.
2>/dev/null:

Redirects standard error (file descriptor 2) to /dev/null, effectively silencing any error messages from the find or tar commands.
In summary, this command searches the user's home directory for files with the names sample.txt, SD.docx, input.txt, or samplesong.mp3, and if any of these files are found, it creates a tarball named temp.tar.gz containing them. Error messages from this operation are silenced by redirecting them to /dev/null.
            
            */

            
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
            
            // printf("gettargz command is %s",command);
            
            /*
            This command uses the find utility to search for files with specific extensions in the user's home directory and archives them into a tarball (compressed archive) named temp.tar.gz. Here's an explanation of each part of the command:

find ~ -type f \( -name "*.c" -o -name "*.txt" -o -name "*.sh" -o -name "*.js" -o -name "*.java" -o -name "*.cpp" \):

find is a command-line utility to search for files and directories in a directory hierarchy.
~ specifies the user's home directory as the search starting point.
-type f filters the search results to include only files.
\( and \) are used to group the expressions within, allowing for a more complex search pattern.
-name "*.c", -name "*.txt", -name "*.sh", -name "*.js", -name "*.java", and -name "*.cpp" specify the file extensions to search for (C, Text, Shell, JavaScript, Java, and C++ files, respectively). Each file extension is connected with the -o operator, which means "or", allowing the search to include any of the specified file extensions.
-exec tar -cf temp.tar.gz {} +:

-exec is an action in the find command that allows you to execute a specific command for each file found by the search.
tar -cf temp.tar.gz {} creates a tarball (compressed archive) of the found files.
-cf temp.tar.gz specifies the archive file to be created (temp.tar.gz).
{} is a placeholder that gets replaced by the file name found by the find command for each matched file.
+ is used to indicate that the tar command should be called once with all the matched files as arguments, rather than once per matched file. This ensures that all matched files are added to the same archive.
2>/dev/null:

Redirects standard error (file descriptor 2) to /dev/null, effectively silencing any error messages from the find or tar commands.
In summary, this command searches the user's home directory for files with the extensions .c, .txt, .sh, .js, .java, or .cpp, and if any of these files are found, it creates a tarball named temp.tar.gz containing them. Error messages from this operation are silenced by redirecting them to /dev/null.
            
            
            */


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
    //new sock will connect with the client from where all the data from client will be received
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
        if(listen(sockfdB, 5) == 0){//server waiting to receive new client 
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
            new_sock = accept(sockfdA, (struct sockaddr*)&new_addr, &addr_size); //accept the connection
            // new_sock will stores the client ID
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