/* The main source code of OCSH mini-shell */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS = 64;

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

        //
        char* args[MAX_ARGS];



        fprintf(stdout, "%s: Command not found.\n", cmd);
    }

}