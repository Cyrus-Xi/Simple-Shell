/* CSC 343
 * Shell (V2) assignment
 *
 * By Cyrus Xi with help from: 
 *   various internet resources (enumerated further in documentation doc).
 *
 * Compilation: gcc -Wall -Wextra -o ezUse2 ezUse2.c
 * Invocation: ezUse2
 * 
 * Summary:
 * 	 Rudimentary shell, fulfills requirements of V1 and those of V2 
 *   (including redirection), with the exception of piping for the latter.
 *
 * Caveats:
 *   No warnings.
 *   Fixed the built-in/fork logic inversion in v1 which Dr. Oldham noted. 
 * 
 *   When exiting using ctrl-d, must press <Enter> one more time to exit.
 *   Can't seem to fix this without causing more errors (eg, ctrl-d 
 * 	 exiting out of terminal itself as well). Using (!feof(stdin)) in 
 *   while loop check didn't really work either.
 * 
 *   Everything else seems to work correctly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 2024
#define ARR_SIZE 2024
#define NAV_LIMIT 8  /* Size of visited directories list */
#define PATH_MAX 128  /* max length of pathname */

void parse_args(char *buffer, char** args, size_t args_size, size_t *num_args) {
    /* buffer is stdin, args is an array of char pointers to the parsed args,
     * args_size is an unsigned int (to represent size), num_args is also used
     * to represent size (and will be modified here)
     */
    char *buf_args[args_size];
    char **cp;
    char *wbuf;
    size_t i, j;
    
    wbuf = buffer;  /* wbuf is assigned stdin */
    buf_args[0] = buffer; /* First pointer points to stdin buffer */
    args[0] = buffer; /* Same as above */
    
    for (cp = buf_args; ( *cp = strsep(&wbuf, " \n\t") ) != NULL;) {
        /* Sets value at cp's indices to be each successive token in wbuf which is
         * separated by delimiters (space, newline, and tab) until the end of the
         * buffer is reached and strsep returns NULL (third statement in for loop
         * condition left empty)
         */
        if ( (*cp != '\0') && (++cp >= &buf_args[args_size]) )
            break;
    }
    
    for (j = i = 0; buf_args[i] != NULL; i++) {
        if ( strlen(buf_args[i]) > 0 ) /* strsep returns "" for successive delims */
            args[j++]=buf_args[i]; /* Copy over buf_arg to args */
    }
    
    *num_args = j; /* After increments, j is now equal to the number of arguments */
    args[j] = NULL; /* Null-terminate array of arguments */
}

int main(void) {
	/* Before, had these as main's parameters: int argc, char *argv[], char *envp[]. */
    char buffer[BUFFER_SIZE];
    char *args[ARR_SIZE]; /* args is array of pointers to chars */
    char currPath[PATH_MAX];
    char *visitedDirs[NAV_LIMIT];  /* Array to hold visited directories */
    char *cdCmd;
    int cdSuccess;
    
    uint i;
    int j;
    
    int fd; /* File descriptor */
    char *ifRedirect = "using("; /* To check if redirecting */
    char *altIn;
    char *altOut;
    char *altErr;
    
    /* Will be like args except without the redirection parameters 
     * (eg, removing "using(in, out, err)"), need it to run commands with
     * options (eg, "cat -v -e").
     */
    char *redirArgs[ARR_SIZE]; 
    
    int visitedDirNext = 0;  /* Index of next (empty) slot in visitedDirs */
    
    /* Index of current slot in visitedDirs. Different from above
     * because of the built-ins 'back' and 'forward'.
     */
    int visitedDirCurr = 0;
     
    int *ret_status = NULL;
    size_t num_args;
    pid_t pid;
    size_t ln;
    
    getcwd(currPath, PATH_MAX); /* currPath is now the current working directory */
    printf("%s@%s --> ", getlogin(), currPath); /* Eg, "cyrus.xi@/home --> " */
    
    /* Returns NULL when EOF reached (ctrl-d)
     * And stores stdin in buffer
     */
    while( fgets(buffer, BUFFER_SIZE, stdin) ) { 
        parse_args(buffer, args, ARR_SIZE, &num_args); /* stdin is now parsed into arguments */
        
        /* See exactly what the args are
        for (i = 0; i < num_args; i++) {
			printf("%s\n", args[i]);
		}
		*/
		
        if (num_args == 0) continue; /* Start over loop and re-prompt */
        if ( strcmp(args[0], "exit") == 0 ) exit(0);
        
        if ( (strcmp(args[0], "cd" ) == 0) || (strcmp(args[0], "back" ) == 0) || (strcmp(args[0], "forward" ) == 0) ) {
			/* If built-in, no need to fork. */
			if ( strcmp(args[0], "cd" ) == 0) { /* cd command */
				if (num_args == 1) chdir(getenv("HOME")); /* Means empty dir, go home */
				else { /* Directory argument provided */
					cdCmd = args[1];
					ln = strlen(cdCmd) - 1;
					if ( (cdCmd[ln] == '\n') || (cdCmd[ln] == '\r'))  {
						cdCmd = '\0'; /* Take out extra chars and null-terminate */
					}
					
					/* Use chdir to emulate cd behavior.
					 * If chdir fails (eg, no such dir name), print err.
					 */
					if ( (cdSuccess = chdir(cdCmd)) == -1 ) { 
						puts(strerror(errno));
					}
				}         
				if (cdSuccess != -1) {     
					/* If worked, store current directory in visitedDirs. */
					visitedDirs[visitedDirNext] = getcwd(visitedDirs[visitedDirNext], PATH_MAX);
					//printf("%s\n", visitedDirs[visitedDirNext]);
					++visitedDirNext;
					visitedDirCurr = visitedDirNext - 1; /* Current slot is next slot - 1 */
				}
            }
            else {
              if ( strcmp(args[0], "back") == 0 ) { /* back command */
                if (visitedDirCurr < 1) {
                  printf("ERROR: Already at the very first visited directory.\n");
                }
                else {
                  --visitedDirCurr; /* Change current slot */
                  chdir(visitedDirs[visitedDirCurr]); /* Use full pathname to go back */
                }
              }
              else {
                if (strcmp(args[0], "forward") == 0) { /* forward command */
                  if (visitedDirCurr >= visitedDirNext - 1) {
                    printf("ERROR: Already at the current, last visited directory.\n");
                  }
                  else {
                    ++visitedDirCurr; /* Change current slot */
                    chdir(visitedDirs[visitedDirCurr]); /* Use full pathname to go forward */
                  }
                }
			  }
		    }
		}
		else { /* If not built-in, fork. */
			pid = fork();
			if (pid) { /* Parent context */
				//printf("Waiting for child (%d)\n", pid);
				pid = wait(ret_status);
				//printf("Child (%d) finished\n", pid);
			} 
            else { /* Child context */
				/* Checks if "using(" is in first arg; if so, do redirection. */
				if ( strstr(args[0], ifRedirect) != NULL ) {
					//printf("%s\n", "Redirecting ... ");
					/* Clean up redirection args. */
					args[0] += 6; /* Move pointer to remove "using(" from 1st arg */
					for (j = 0; j < 3; j++) {
						/* If "*", then don't need to clean. */
						if (strcmp(args[j], "*") != 0) { 
							ln = strlen(args[j]);
							args[j][ln-1] = '\0'; /* "Remove" last char */
						}
						//printf("%s\n", args[j]);
					}
					
					for (j = 0,i = 3; i < num_args; i++,j++) {
						redirArgs[j] = args[i];
					}
					/* See what redirArgs are.
					for (i = 0; i < num_args-3; i++) {
						printf("%s\n", redirArgs[i]);
					}
					*/
					
					/* Reassign redirection args for convenience. */
					altIn = args[0];
					altOut = args[1];
					altErr = args[2];
					//printf("%s %s %s\n", altIn, altOut, altErr);
					
					/* Redirect stdin. */
					if ( strcmp(altIn, "*") != 0 ) {
						printf("\n%s\n\n", "Redirecting stdin.");
						close(0); /* Release fd num 0 */
						
						/* Open a file with fd num 0, set permissions appropriately,
						 * make sure it works.
						 */
						if ( (fd = open(altIn, O_RDWR, 0777)) == -1 ) { 
							perror("Error opening the file:");
							exit(1);
						}
						if ( execvp(redirArgs[0], redirArgs) ) { /* Execute command */
							puts(strerror(errno));
							exit(127);
						}
					}
					/* Redirect stdout. */
					else if ( strcmp(altOut, "*") != 0 ) { 
						printf("\n%s\n\n", "Redirecting stdout.");
						close(1); /* Release fd num 1 */
						
						/* Open a file with fd num 1, set permissions appropriately,
						 * make sure it works.
						 */
						if ( (fd = open(altOut, O_RDWR|O_CREAT, 0777)) == -1 ) { 
							perror("Error opening the file:");
							exit(1);
						}
						
						if ( execvp(redirArgs[0], redirArgs) ) { /* Execute command */
							puts(strerror(errno));
							exit(127);
						}
					}
					/* Redirect stderr. */
					else if ( strcmp(altErr, "*") != 0 ) {
						printf("\n%s\n\n", "Redirecting stderr.");
						close(2); /* Release fd num 2 */
						
						/* Open a file with fd num 2, set permissions appropriately,
						 * make sure it works.
						 */
						if ( (fd = open(altErr, O_RDWR|O_CREAT, 0777)) == -1 ) { 
							perror("Error opening the file:");
							exit(1);
						}
						if ( execvp(redirArgs[0], redirArgs) ) { /* Execute command */
							exit(127);
						}
					}
					else { /* All "parameters" default, ie, all "*" */
						printf("%s\n", "No redirection");
						if ( execvp(redirArgs[0], redirArgs) ) { /* Execute command */
							puts(strerror(errno));
							exit(127);
						}
					}
					
				}
				/*
				else {
					printf("Not redirecting!");
				}
				*/
				else if ( execvp(args[0], args) ) { /* Execute command */
					puts(strerror(errno));
                    exit(127);
                }
            }
        }  
        getcwd(currPath, PATH_MAX); /* currPath is now the current working directory */
		printf("%s@%s --> ", getlogin(), currPath); /* Eg, "cyrus.xi@/home --> " */
    }
    return 0;
}
