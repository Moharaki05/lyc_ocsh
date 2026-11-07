/* The main source code of OCSH mini-shell */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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
			if (dir==NULL || chdir(dir) != 0)
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

        if(histCount < MAX_HISTORY)
        	history[histCount++] = strdup(cmd);
		
        char* segments[MAX_CMDS];
        int nseg = splitPipeline(cmd, segments);
	
		for(int i = 0; i < nseg; i++){
			char* args[MAX_ARGS];
			int n = splitArgs(segments[i], args);
			if (n == 0) continue;

			if(runBuiltins(args, n)) continue;
			resolveAlias(args);
			
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
