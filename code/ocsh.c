/* The main source code of OCSH mini-shell */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64

int main(void){
    //Command Buffer
    char cmd[1024];
    // Flush after every printf
    setbuf(stdout, NULL);

    // Main REPL
    while (1) {
        fprintf(stdout, "ocsh> ");

        //Read the user's inputs
        if(fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        
        //strcspn looks for the first occurence of an (or multiple) 
        //unauthorized character(s) in a string
        //and returns the char number before it.
        int newLineIndex = strcspn(cmd, "\n");
        cmd[newLineIndex] = '\0';
        if(cmd[0] == '\0') continue;

		//TODO : Split cmd using "->" into other commands.
		
        //strtok splits command into words, using spaces and tabs as separators
        //The first call (strtok(cmd,...)) returns the first word
        //Next calls return the next words, then NULL when none are left.
        char* args[MAX_ARGS];
		int n = 0;
		for (char *tok = strtok(cmd, "\t"); tok && n < MAX_ARGS - 1; tok = strtok(NULL, "\n"))
			args[n++] = tok;
		args[n] = NULL;
		
		pid_t pid = fork();
		if(pid == 0){
			//Child process
			execvp(args[0], args);
			
			//only if execvp fails
			fprintf(stdout, "%s: Command not found.\n", cmd); 
			exit(127); //127 is the default error code for command not found		
		}
		else if (pid > 0){
			//Parent process
			waitpid(pid, NULL, 0);
		}
		else {
			perror("fork");
		}

    }
	return 0;
}
