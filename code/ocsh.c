/* The main source code of OCSH mini-shell */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_BUFFER 1024
#define MAX_ARGS 64
#define MAX_CMDS 16
#define MAX_HISTORY 100

// Command history
char *history[MAX_HISTORY];
int histCount = 0;

// Alias table: ocsh names -> real Linux commands
struct Alias {
	const char *name;
	const char *realCmd;
};
struct Alias aliases[] = {
	{"list", "ls"},
	{"show", "cat"},
	{"copy", "cp"},
	{"move", "mv"},
	{"del", "rm"},
	{"newdir", "mkdir"},
	{"newfile", "touch"},
};
int aliasCount = sizeof(aliases) / sizeof(aliases[0]);

//If args[0] is an alias, replace it by the real command
void resolveAlias(char *args[]){
	for(int i = 0; i < aliasCount; i++){
		if(strcmp(args[0], aliases[i].name) == 0){
			args[0] = (char *)aliases[i].realCmd;
			return;
		}
	}
}
//Built-ins
enum Builtin {
	B_NONE,
	B_GO,
	B_QUIT,
	B_PATH,
	B_SAY,
	B_INFO,
	B_PAST,
	B_OSINFO
};

enum Builtin getBuiltin(const char *name){
	if(strcmp(name, "go") == 0) return B_GO;
	if(strcmp(name, "quit") == 0) return B_QUIT;
	if(strcmp(name, "path") == 0) return B_PATH;
	if(strcmp(name, "say") == 0) return B_SAY;
	if(strcmp(name, "info") == 0) return B_INFO;
	if(strcmp(name, "past") == 0) return B_PAST;
	if(strcmp(name, "osinfo") == 0) return B_OSINFO;
	return B_NONE;
}

struct Info {
	const char *name;
	const char *text;
};
struct Info infoTable[] = {
	{"go", "go [dir]	change the current directory (HOME if no dir)"},
	{"quit", "quit	exit the shell"},
	{"path", "path	print the current directory"},
	{"say", "say [text]	print text"},
	{"info", "info [cmd]	show help about a builtin"},
	{"past", "past	show the command history)"},
	{"osinfo", "osinfo	show course information"},
};
int infoCount = sizeof(infoTable) / sizeof(infoTable[0]);

int runBuiltins(char *args[], int n){
	switch(getBuiltin(args[0])){
		case B_GO: {
			const char *dir = (n > 1) ? args[1] : getenv("HOME"); //goes to the wanted directory if given, goes to home otherwise
			if (dir == NULL || chdir(dir) != 0)
				perror("go");
			return 1;
		}
		case B_QUIT: 
			exit(0);
		
		case B_PATH: {
			char buf[1024];
			if(getcwd(buf, sizeof(buf)) != NULL) //check if the current working dir exists
				fprintf(stdout, "%s\n", buf);
			else
				perror("path");
			return 1;
		}
		case B_SAY: 
			for(int i = 1; i < n; i++)
				fprintf(stdout, "%s%s", args[i], (i<n-1) ? " " : "");
			fprintf(stdout, "\n");
			return 1;
		
		case B_INFO:
			if(n == 1) {
				//No args : list every builtin
				for (int i = 0; i < infoCount; i++){
					fprintf(stdout,"%s\n", infoTable[i].text);
				}
			}
			else {
				//Argument : help about one builtin
				for (int i = 0; i < infoCount; i++){
					if(strcmp(args[1], infoTable[i].name) == 0){
						fprintf(stdout, "%s\n", infoTable[i].text);
						return 1;
					}
				}
				fprintf(stderr, "info: no help for '%s'\n", args[1]);
			}
			return 1;
		
		case B_PAST:
			for (int i = 0; i < histCount; i++)
				fprintf(stdout, "%4d	%s\n", i + 1, history[i]);
			return 1;
			
		case B_OSINFO:
			fprintf(stdout,"Systèmes d'exploitation -- UIR/ESIN\n");
			return 1;	
			
		default:
			return 0; //not a builtin
	}
}

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

//Remove "<", ">" and ">>" (and the file name after each) from args.
//Fills inFile/outFile and sets append to 1 for ">>".
//Returns -1 on syntax error
int parseRedirections(char *args[], int *n, char **inFile, char **outFile, int *append){
	int j = 0; //writing index
	*inFile = NULL;
	*outFile = NULL;
	*append = 0;

	for(int i = 0; i < *n; i++){
		if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0){
			if (i + 1 >= *n){ //checks if the word at i+1 is the last word
				fprintf(stderr, "ocsh: Syntax error: Missing file name after '%s'\n", args[i]);
				return -1;
			}
			if (args[i][0] == '<'){ //if the second "word" is "<", we keep args[i+1] in inFile
				*inFile = args[i+1];
			}
			else { //else, we keep it in outFile
				*outFile = args[i+1];
				*append = (args[i][1] == '>'); //true : ">>" -> append, false : ">"
			}
			i++;
		}
		else {
			args[j++] = args[i]; //copy the arg at index j then increment j
		}
	}
	args[j] = NULL;
	*n = j;
	return 0;
}

//Redirect stdin/stdout to the given files. Returns -1 if a file can't be opened
int applyRedirections(const char *inFile, const char *outFile, int append){
	if(inFile){
		int fd = open(inFile, O_RDONLY);
		if (fd < 0) {
			perror(inFile);
			return -1;
		}
		dup2(fd, STDIN_FILENO); //fd 0 now points to the file
		close(fd); //the extra descriptor is no longer needed
	}
	if(outFile){
		int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
		int fd = open(outFile, flags, 0644);
		if (fd < 0) {
			perror(outFile);
			return -1;
		}
		dup2(fd, STDOUT_FILENO); //fd 1 now points to the file
		close(fd);
	}
	return 0;
}

void runPipelines(char *segments[], int nseg){
	char *argv[MAX_CMDS][MAX_ARGS];
	int argc_[MAX_CMDS];
	char *inFile[MAX_CMDS], *outFile[MAX_CMDS];
	int append[MAX_CMDS];

	//Parses every segment first; abort the whole pipeline on error
	for (int i = 0; i < nseg; i++) {
		argc_[i] = splitArgs(segments[i], argv[i]);
		if (parseRedirections(argv[i], &argc_[i], &inFile[i], &outFile[i], &append[i]) < 0) return;
		if (argc_[i] == 0){
			fprintf(stderr, "osch : Syntax error: empty command near '->'\n");
			return;
		}
	}

	//Creates the pipes and one child per command
	pid_t pids[MAX_CMDS];
	int started = 0;
	int prevRead = -1; //red end of previous pipe (-1 for the first command)

	for (int i = 0; i < nseg; i++) {
		int fd[2] = {-1, -1}; // stays -1 for the last command (no pipe after it)
		if (i < nseg - 1 && pipe(fd) < 0) {
			perror("pipe");
			break;
		}

		pid_t pid = fork();
		if (pid < 0){
			if (fd[0] != -1) {
				close(fd[0]);
				close(fd[1]);
			}
			break;
		}

		if (pid == 0) {
			//Child process : plug stdin on previous pipe, stdout on the next one.
			if (prevRead != -1){
				dup2(prevRead, STDIN_FILENO);
				close(prevRead);
			}
			if (fd[1] != -1){
				dup2(fd[1], STDOUT_FILENO);
				close(fd[0]);
				close(fd[1]);
			}

			//Explicit redirections override the pipe
			if (applyRedirections(inFile[i], outFile[i], append[i] < 0))
				exit(EXIT_FAILURE);

			//A builtin inside a pipe runs in the child
			if (runBuiltins(argv[i], argc_[i]))
				exit(EXIT_SUCCESS);

			resolveAlias(argv[i]);
			execvp(argv[i][0], argv[i]);
			fprintf(stderr, "%s: Command not found.\n", argv[i][0]);
			exit(127);
		}

		//Parent process : remember the child, close the ends it doesn't need anymore
		pids[started++] = pid;
		if (prevRead != -1) close(prevRead);
		if (fd[1] != -1) close(fd[1]);
		prevRead = fd[0]; //becomes the stdin source of the next child
	}
	
	if (prevRead != -1) close(prevRead); // only reached after a break;

	//Wait for all children, only after all of them are started
	for(int k = 0; k < started; k++)
		waitpid(pids[k], NULL, 0);
}

int main(void){
    //Command Buffer
    char cmd[MAX_BUFFER];
    //Flush after every printf
    setbuf(stdout, NULL);

	//Move cursor to top-left, then clear the screen
    fprintf(stdout,"\033[H\033[2J");

    //Main REPL
    while (true) {
        fprintf(stdout, "ocsh> ");

        //Read the user's inputs
        if(fgets(cmd, sizeof(cmd), stdin) == NULL){
        	//fprintf(stdout, "\n");
        	puts("");
        	break;	
        }
        
        //strcspn looks for the first occurence of an (or multiple) 
        //unauthorized character(s) in a string
        //and returns the char number before it.
        int newLineIndex = strcspn(cmd, "\n");
        cmd[newLineIndex] = '\0';
        if(cmd[0] == '\0') continue;

        if(histCount < MAX_HISTORY)
        	history[histCount++] = strdup(cmd);
		
        char* segments[MAX_CMDS];
        int nseg = splitPipeline(cmd, segments);

		if (nseg == 1) {
			char* args[MAX_ARGS];
			char *inFile, *outFile;
			int n = splitArgs(segments[0], args);
			int append;
			if(parseRedirections(args, &n, &inFile, &outFile, &append) < 0) continue;
			if (n == 0) continue;
			
			if (getBuiltin(args[0]) != B_NONE){
				int savedIn = dup(STDIN_FILENO);
				int savedOut = dup(STDOUT_FILENO);
				if (applyRedirections(inFile, outFile, append) == 0)
						runBuiltins(args, n);
					dup2(savedIn, STDIN_FILENO);
					dup2(savedOut, STDOUT_FILENO);
					close(savedIn);
					close(savedOut);
					continue;
				}
							
				resolveAlias(args);
						
				pid_t pid = fork();
					if(pid == 0){
						//Child process
						if(applyRedirections(inFile, outFile, append) < 0)
							exit(1);
						execvp(args[0], args);
									
						//only if execvp fails
						fprintf(stderr, "%s: Command not found.\n", args[0]); 
						exit(127); //127 is the default error code for command not found		
					}
					else if (pid > 0){
						//Parent process
						wait(NULL);
					}
					else {
						perror("fork");
					}
		}
		else {
			runPipelines(segments, nseg);
		}        
    }
}
