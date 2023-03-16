#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/* this small guide is meant to provide some basic pipe() implementation and general knowledge on how it works */

void	simplest_pipe();
void	fairly_harder_pipe();
void	classic_bash_pipes();
void	another_classic_example();
void	understand_multiple_pipes();
void	recursive_fancyness(char* const* cmds[], size_t pos, int in_fd);

int main()
{
	printf("\nExample 1\n");
	simplest_pipe();
	printf("------------------------------------------------------------------------------\n\n");
	printf("Example 2\n");
	fairly_harder_pipe();
	printf("------------------------------------------------------------------------------\n\n");
	printf("Example 3\n");
	classic_bash_pipes();
	printf("------------------------------------------------------------------------------\n\n");
	printf("Example 4\n");
	another_classic_example();
	printf("------------------------------------------------------------------------------\n\n");
	printf("Example 5\n");
	understand_multiple_pipes();
	printf("------------------------------------------------------------------------------\n\n");
	printf("Example 6\n");
	printf("This is a small example on how recursive pipe is possible, just to show that there's no one single way to do them\n");
	printf("We will execute the command before, calling this function recursively until the last command\n\n");
	const char *args1[] = { "/bin/echo", "pipes", NULL };
	const char *args2[] = { "/usr/bin/tr", "-d", "e", NULL };
	const char *args3[] = { "/usr/bin/tr", "-d", "i", NULL };
	const char* const* cmd[] = { args1, args2, args3, NULL };
	recursive_fancyness((char* const**)cmd, 0, STDIN_FILENO);
	printf("------------------------------------------------------------------------------\n\n");
	printf("Lunching lsof with system() to check for unclosed pipes ;)\n");
	system("lsof | grep pipes");
}

void simplest_pipe()
{
	printf("Simplest pipe possible, 1 pipe only\n\n");

	int end[2];

	pipe(end);

	// we're writing inside end[1], it will be accessible from end[0]
	write(end[1], "I'm pipe 1", 16);
	close(end[1]);

	// now we're accessing data from end[0]
	char buffer[256];

	if (read(end[0], buffer, sizeof buffer) < 0) {
		perror("read");
		return ;
	}
	printf("Read from pipe 0: %s\n\n", buffer);
	close(end[0]);
}

void	fairly_harder_pipe()
{
	printf("Same result as before, but using dup2() to set end[1] as new STDOUT_FILENO\n\n");

	// int dup2(int fildes, int fildes2);
	// The dup2() function returns a descriptor with the value fildes2.
	// The descriptor refers to the same file as fildes, and it will close the file that fildes2 was associated with

	int stdout_cpy = dup(1); // we need to store the original STDOUT_FILENO to recover it after finish piping
	int end[2];

	pipe(end);

	// we'll set end[1] as STDOUT_FILENO using dup2().
	dup2(end[1], STDOUT_FILENO);
	close(end[1]);
	// this time we will write directly into 1
	write(1, "I'm pipe 1", 16);

	// retrieving data from pipe[0]
	char buffer[256];

	if (read(end[0], buffer, sizeof buffer) < 0) {
		perror("read");
		return ;
	}

	dup2(stdout_cpy, STDOUT_FILENO); // recovering STDOUT_FILENO on the original fd associated with it
	close(end[0]); // we don't need this
	close(stdout_cpy);
	printf("Read from pipe 0: %s\n\n", buffer);
}

void	classic_bash_pipes()
{
	printf("Now we'll execute a simple command, send output to the pipe and exec another command on this output.\nFinally, we will print the result on the STDOUT_FILENO.\nCommand: ls -l | wc -l\n\n");

	int stdout_cpy = dup(1); // STDOUT cpy
	int stdin_cpy = dup(0); // STDIN cpy	
	int end[2];
	extern char **environ; // we need environ to pass to execve

	pipe(end);

	// we need to fork in other to execute commands. execve closes the current process on success
	// child
	if (!fork()) {
		close(end[0]); // we don't need this pipe right now, so we're closing it to avoid fd leaks
		dup2(end[1], STDOUT_FILENO);
		close(end[1]);
		char *args[3] = {"/bin/ls", "-l", NULL};
		execve(args[0], args, environ);
		perror("execve failed"); // if execve fail we print the error and exit(0)
		exit (0);
	}
	// parent
	waitpid(-1, 0, 0);
	close(end[1]); // we don't need this anymore
	// new child for execve
	if (!fork()) {
		dup2(end[0], STDIN_FILENO); // we're getting the output of the first command from pipe[1]
		close(end[0]);
		char *args[3] = {"/usr/bin/wc", "-l", NULL};
		execve(args[0], args, environ);
		perror("execve failed");
		exit (0);
	}
	waitpid(-1, 0, 0);
	close(end[0]);
	close(stdin_cpy);
	close(stdout_cpy);
}

void	another_classic_example()
{
	printf("Now we're doing the same thing as before but in a slighty cleaner way, expecting the same output\n\n");

	int stdout_cpy = dup(1); // STDOUT cpy
	int stdin_cpy = dup(0); // STDIN cpy
	int end[2];
	extern char **environ; // we need environ to pass to execve

	pipe(end);

	if (!fork()) {
		// child
		if (!fork()) {
			close(end[0]);
			dup2(end[1], STDOUT_FILENO);
			char *args[3] = {"/bin/ls", "-l", NULL};
			execve(args[0], args, environ);
			perror("execve failed"); // if execve fail we print the error and exit(0)
			exit (0);
		}
		// parent
		waitpid(-1, 0, 0);
		close(end[1]);
		dup2(end[0], STDIN_FILENO); // we're getting the output of the first command from pipe[1]
		close(end[0]);
		char *args[3] = {"/usr/bin/wc", "-l", NULL};
		execve(args[0], args, environ);
		perror("execve failed");
		exit (0);
	}
	close(end[1]);
	waitpid(-1, 0, 0);
	close(end[0]);
	close(stdin_cpy);
	close(stdout_cpy);
}

void	understand_multiple_pipes()
{
	printf("Let's try to work with multiple pipes and understand the process!\nCommand: echo pipes | tr -d e | tr -d i\n\n");
	printf("There are multiple way to perform this task. Here we will simply using 2 pipes(1 for every '|').\nIt could be pipes[4] or pipes[2][2] and so on. Just keep in mind that every call to pipe() generate 2 pipes, so for every pipe you need to call pipe()\n\n");

	int stdout_cpy = dup(1); // STDOUT cpy
	int stdin_cpy = dup(0); // STDIN cpy
	int end[4];
	extern char **environ; // we need environ to pass to execve

	pipe(&end[0]); // piping pipe 0-1
	pipe(&end[2]); // piping pipe 2-3

	int i = 0;
	char *args1[4] = { "/bin/echo", "pipes", NULL };
	char *args2[4] = { "/usr/bin/tr", "-d", "e", NULL };
	char *args3[4] = { "/usr/bin/tr", "-d", "i", NULL };
	char **cmd[4] = { args1, args2, args3 };
	while (i < 3)
	{
		if(!fork()) {
			close(end[i * 2]);
			if (i == 2)
				dup2(stdout_cpy, STDOUT_FILENO); // if we're the last command we're printing on STDOUT
			else
				dup2(end[(i * 2) + 1], STDOUT_FILENO); // else we're printing inside the pipe
			close(end[(i * 2) + 1]);
			execve(cmd[i][0], cmd[i], environ);
			perror("execve failed");
			exit(0);
		} else { // parent
			dup2(end[i * 2], STDIN_FILENO); // we're setting the pipe as STDIN
			close(end[i * 2]); // closing unecessary pipes
			close(end[i * 2 + 1]);
		}
		i++;
	}
	i = 0;
	while (i++ < 3)
		waitpid(-1, 0, 0);
	// we're waiting for every child to exec the command. It's very important to put the waitpid() at the end of the pipes and not in the loop!
	// pipes are asynchronous, it means that when we execve a command, it's being stucked inside the pipe waiting for some input from STDIN (in our case the pipe)
	// putting waitpid at the end progressively fill the STDIN of every command
	dup2(stdin_cpy, STDIN_FILENO);
	close(stdin_cpy);
	close(stdout_cpy);
}

void	recursive_fancyness(char* const* cmds[], size_t pos, int in_fd)
{
	extern char **environ;

	// this is the final command, we're checking it at the beginning
	if (cmds[pos + 1] == NULL) {
		if(!fork()) {
    		dup2(in_fd, STDIN_FILENO); 
			close(in_fd);
    		execve(cmds[pos][0], cmds[pos], environ);
			perror("execve failed");
		}
		size_t i = 0;
		while (i++ < pos)
			waitpid(-1, 0, 0); // waiting for all childs to end
  	} else {
    	int end[2];
		pipe(end);
    	if (!fork()) {
    		close(end[0]);
    		dup2(in_fd, STDIN_FILENO);
			close(in_fd);
    		dup2(end[1], STDOUT_FILENO);
			close(end[1]);
    		execve(cmds[pos][0], cmds[pos], environ);
    		perror("execve failed");
		}
    	close(end[1]);
		if (pos > 0)
    		close(in_fd); // we don't wanna close this the first time becuase it's our STDIN
    	recursive_fancyness(cmds, pos + 1, end[0]);
		close(end[0]); // we want to be sure there won't be any fd leak!
	}
}
