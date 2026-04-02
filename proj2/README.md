**Makefile**
Unchanged to however it was created with the original zip file.

**proj2_server.cc**
*Includes*
pthread.h: Allows for the creation of threads.
csignal: Allows to copy memory so it can be used for parsing a message to a thread.
proj2/lib/*.h: All of the headers provided were helpful when coding.

*ParseMessage*
Parameters
- msg: The files the client has sent.
- client_addr: Gets the memory address of the client.
- file_paths: The directory of the files.
- rows_per_file: The total amount of lines within a file.

Function
- Gets the data of msg and turns it into a character pointer (c_ptr), and initializes it with an offset of 0.
- Obtains the client's address by server sending bytes back to client with its pointer and offset, then gets back the content from client.
- Server counts the amount of files client has.
- Server browses through each file and gets its path size, path content, and returns the number of lines in the file back into rows_per_file.

*main*
Parameters
- argc: Terminal argument word count.
- argv[]: The words of that terminal argument.

Function
- Initializes the server and its resources based off of the arguments from terminal.
- Looks for a request from client with a packet size limit of 65,535 bytes max.
- Once request is found, it moves the request into a thread, obtains its ID, creates it, and puts it through StartRoutine.
- Does some memory clean up by detaching the thread from its ID within the serverif StartRoutine is successful, else thread is deleted from memory if it fails.

*StartRoutine*
- Parses the messsage data then deletes message from memory to free space, and obtains the amount of lines it has.
- Uses a semaphore to bring down ShaSolvers and FileReaders to get the 64 hexadecimal string from within the contents of the file.
- A hash gets created, resized, and then gets filled with the processes of that file.
- The has gets flattened into a single payload, which gets written back to the client.
- ShaSolvers and FileReaders comes up from the semaphore so that other threads can use those same handlers.