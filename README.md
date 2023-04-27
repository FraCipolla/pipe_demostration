# pipe_demostration
Simple demostration of system call pipe()

Compile with gcc -Wall -Werror -Wextra pipe.c -o pipes

This is an example of different pipe() implementation.

From Linux man page:

pipe() 
       creates a pipe, a unidirectional data channel that can be
       used for interprocess communication.  The array pipefd is used to
       return two file descriptors referring to the ends of the pipe.
       pipefd[0] refers to the read end of the pipe.  pipefd[1] refers
       to the write end of the pipe.  Data written to the write end of
       the pipe is buffered by the kernel until it is read from the read
       end of the pipe.

For this purpose, we will use system call function dup2(), to substitute the fd generated from pipe() to the STDIN or STDOUT
and the dup() call to save and restore the original STDIN and STDOUT.

Again, from linux man page:
int dup(int oldfd);
int dup2(int oldfd, int newfd);

dup()
        system call allocates a new file descriptor that refers
        to the same open file description as the descriptor oldfd.
dup2()
        The dup2() system call performs the same task as dup(), but
        instead of using the lowest-numbered unused file descriptor, it
        uses the file descriptor number specified in newfd.  In other
        words, the file descriptor newfd is adjusted so that it now
        refers to the same open file description as oldfd.
        
To understand the outputs generated from the program, check the code and read the comments.
After the last test, the program will lunch lsof to check for unclosed pipes.

Now compile and run the program!!
