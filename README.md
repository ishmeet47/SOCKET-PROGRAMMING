# SOCKET-PROGRAMMING

In this client-server project, a client can request a file or a set of files from the server. The
server searches for the file/s in its file directory rooted at its ~ and returns the tar.gz of the
file/files requested to the client (or an appropriate message otherwise). Multiple clients can
connect to the server from different machines and can request file/s as per the following commands :

List of Client Commands:

 **findfile filename**
o If the file filename is found in its file directory tree rooted at ~, the server must
return the filename, size(in bytes), and date created to the client and the
client prints the received information on its terminal.
 Note: if the file with the same name exists in multiple folders in the
directory tree rooted at ~, the server sends information pertaining to
the first successful search/match of filename
 Else the client prints “File not found”
o Ex: C$ findfile sample.txt

 **sgetfiles size1 size2 <-u>**
o The server must return to the client temp.tar.gz that contains all the files in
the directory tree rooted at its ~ whose file-size in bytes is >=size1 and <=size2
 size1 < = size2 (size1>= 0 and size2>=0)
o -u unzip temp.tar.gz in the pwd of the client
o Ex: C$ sgetfiles 1240 12450 -u

 **dgetfiles date1 date2 <-u>**
o The server must return to the client temp.tar.gz that contains all the files in the
directory tree rooted at ~ whose date of creation is >=date1 and <=date2
(date1<=date2)
o -u unzip temp.tar.gz in the pwd of the client
o Ex: C$ dgetfiles 2023-01-16 2023-03-04 -u

 **getfiles file1 file2 file3 file4 file5 file6 <-u >**
o The server must search the files (file 1 ..up to file6) in its directory tree rooted
at ~ and return temp.tar.gz that contains at least one (or more of the listed
files) if they are present 
Page 3 of 4
o If none of the files are present, the server sends “No file found” to the client
(which is then printed on the client terminal by the client)
o -u unzip temp.tar.gz in the pwd of the client
o Ex: C$ getfiles new.txt ex1.c ex4.pdf

 **gettargz <extension list> <-u>** //up to 6 different file types
o the server must return temp.tar.gz that contains all the files in its directory tree
rooted at ~ belonging to the file type/s listed in the extension list, else the
server sends the message “No file found” to the client (which is printed on the
client terminal by the client)
o -u unzip temp.tar.gz in the pwd of client
o The extension list must have at least one file type and can have up to six
different file types
o Ex: C$ gettargz c txt pdf
  
 **quit**
o The command is transferred to the server and the client process is terminated
  
  
  
Also implemented the load balancing using the below constraints : 
  
Alternating Between the Server and the Mirror
 The server and the mirror (the server’s copy possibly with a few
additions/changes) are to run on two different machines/terminals.
  
 The first 4 client connections are to be handled by the server.
  
 The next 4 client connections are to be handled by the mirror.
  
 The remaining client connections are to be handled by the server and the
mirror in an alternating manner- (ex: connection 9 is to be handled by the
server, connection 10 by the mirror, and so on) 
