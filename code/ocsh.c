/* The main source code of OCSH mini-shell */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){
    // Main REPL
    while (1) {
        //Command Buffer
        char cmd[1024];

        // Flush after every printf
        setbuf(stdout, NULL);

        fprintf(stdout, "ocsh> ");

        //Read the user's inputs
        if (argc < 1) {
            fprintf(stderr, "No command entered! Please input a proper one.\n");
            return 0;
        }

        if(fgets(cmd, sizeof(cmd), stdin) == NULL) {
            return 0;
        }
        
        //strcspn looks for the first occurence of an (or multiple) 
        //unauthorized character(s) in a string
        //and returns the char number before it.
        int newLineIndex = strcspn(cmd, "\n");
        cmd[newLineIndex] = '\0';


        fprintf(stdout, "%s: Command not found.\n", cmd);
    }

}