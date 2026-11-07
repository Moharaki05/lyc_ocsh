/* The main source code of OCSH mini-shell */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_CMDS 16

int splitPipeline(char *line, char *segments[]){
 	int count = 0;
   	char *start = line;
   	char *pos;

   	//strstr returns a pointer to the next "->", or NULL if none is left in the command
   	while(count < MAX_CMDS - 1 && (pos = strstr(start, "->")) != NULL){
   		*pos = '\0'; //end the current segment here
   		segments[count++] = start;
   		start = pos + 2; //skips the two characters of "->"
   	}
   	segments[count++] = start; //last segment
   	return count;
}

int splitArgs(char *segment, char *args[]){
   	int n = 0;
   	//strtok splits command into words, using spaces and tabs as separators
   	//The first call (strtok(cmd,...)) returns the first word
   	//Next calls return the next words, then NULL when none are left.
   	for (char *tok = strtok(segment, " \t"); tok && n < MAX_ARGS - 1; tok = strtok(NULL, " \t"))
   		args[n++] = tok;
   	args[n] = NULL;
   	return n;
}

int main(void){
    //Command Buffer
    char cmd[1024];
    // Flush after every printf
    setbuf(stdout, NULL);

    // Main REPL
    while (1) {
        fprintf(stdout, "ocsh> ");

        //Read the user's inputs
        if(fgets(cmd, sizeof(cmd), stdin) == NULL){
        	fprintf(stdout, "\n");
        	break;	
        }
        
        //strcspn looks for the first occurence of an (or multiple) 
        //unauthorized character(s) in a string
        //and returns the char number before it.
        int newLineIndex = strcspn(cmd, "\n");
        cmd[newLineIndex] = '\0';
        if(cmd[0] == '\0') continue;
		
        char* segments[MAX_CMDS];
        int nseg = splitPipeline(cmd, segments);
	
		for(int i = 0; i < nseg; i++){
			char* args[MAX_ARGS];
			int n = splitArgs(segments[i], args);
			if (n == 0) continue;
			
			pid_t pid = fork();
				if(pid == 0){
					//Child process
					execvp(args[0], args);
						
					//only if execvp fails
					fprintf(stderr, "%s: Command not found.\n", args[0]); 
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
    }
	return 0;
}
