#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>

struct node {
	char *name;
	struct node *next;
};

char **tok_helper(const char *str) {
        // pass 1: count number of tokens
        int num_tokens = 0;
        const char *sep = " \n\t";
        char *tmp, *word;
        char *s = strdup(str);
        for ( word = strtok_r(s, sep, &tmp); word != NULL ; word = strtok_r(NULL, sep, &tmp) ) {
                num_tokens++;
        }
        free(s);

        // create the array
        char **token_list = (char**) malloc(sizeof(char*)*(num_tokens+1));

        // pass 2: add tokens to the array
        s = strdup(str);
        int i = 0;
        for ( word = strtok_r(s, sep, &tmp); word != NULL ; word = strtok_r(NULL, sep, &tmp) ) {
                char *token = strdup(word);
                token_list[i] = token;
                i++;
        }
        token_list[i] = NULL;
        free(s);

        return token_list;
}

char ***tokenify(const char *str, int cmds) {
	char ***cmdtokens = NULL;
	if (cmds <= 0 ) {
		return cmdtokens;
	}
	cmdtokens = (char***) malloc(sizeof(char**)*(cmds+1));

	const char *sep = ";";
	char *tmp, *word;
	char *s = strdup(str);
	word = strtok_r(s, sep, &tmp);

	int i=0;
	for (; i < cmds; i++) {
		if (word != NULL) {
			// tokenize and add to array
			cmdtokens[i] = tok_helper(word);
		}
		// get a new "big" token
		word = strtok_r(NULL, sep, &tmp);
	}

	free(s);
	cmdtokens[i] = NULL;
	return cmdtokens;
}

char *strip(char *str) {
	// need to strip the newline character from the end of string
	int len = strlen(str);
	if (str[len-1] == (int)'\n')
		str[len-1] = '\0';
	return str;
	printf("\n%s\n",str);
}

int main(int argc, char **argv) {
	char buffer[1024];
	int par=0;
	int p_running=0;
	int childrv;

        // read shell-config
        FILE *configfile = fopen("shell-config","r");
        struct node *head = NULL;
	struct node *tmp;

	if (configfile != NULL) {
                char filebuffer[256];
                head = (struct node*) malloc(sizeof(struct node));
                head->name = strdup(strip(fgets(filebuffer, 256, configfile)));
                head->next = NULL;

                while (fgets(filebuffer, 256, configfile) != NULL) {
                        tmp = (struct node*) malloc(sizeof(struct node));
                        tmp->name = strip(strdup(filebuffer));
                        tmp->next = head;
                        head = tmp;
                }
        }

	printf("$$ ");
	fflush(stdout);

	while (fgets(buffer, 1024, stdin) != NULL) {
		int cmds, i, j;

		// count the number of commands and locate any comments
		cmds=1;
		i=0;
		for (; i<1024; i++) {
			if ( buffer[i] == (int)'\0' )
				break;
			if ( buffer[i] == (int)';' )
				cmds++;
			if ( buffer[i] == (int)'#' ) {
				buffer[i]='\0';
				break;
			}
		}

		// tokenize commands and organize into array of array of pointers
		char ***cmdtoks = tokenify(&buffer, cmds);
		/*
		// diagnostic tool for printing the command token array
		i=0;j=0;
		for (; i < cmds; i++) {
			printf("Command %i:\n", i);
			j=0;
			while ( cmdtoks[i][j] != NULL ) {
				printf("%s\n", cmdtoks[i][j]);
				j++;
			}
		}
		*/

		// begin executing commands
		i=0; j=0;
		int exit=0;
		pid_t pid=1;

		while (cmdtoks[i]!=NULL) {
			if (cmdtoks[i][0]==NULL)
				break;

			// handle EOF
			if (cmdtoks[i][0][0]==4) {
				printf("\n");
				break;
			}

			// handle mode command
			if (strcmp(cmdtoks[i][0],"mode")==0) {
				if (cmdtoks[i][1] == NULL) {
					if (par) {
						printf("current mode: parallel\n");
					} else {
						printf("current mode: sequential\n");
					}
				} else if (cmdtoks[i][2] == NULL) {
					if (strcmp(cmdtoks[i][1], "sequential")==0 || strcmp(cmdtoks[i][1], "s")==0) {
						if (par==1) {
							// wait for currently running processes to finish so we can start executing sequentially
                                                	while (p_running > 0) {
                                				waitpid(-1, &childrv, 0);
                               					p_running--;
				                        }
						}
						par=0;
					} else if (strcmp(cmdtoks[i][1], "parallel")==0 || strcmp(cmdtoks[i][1], "p")==0) {
						par=1;
					} else {
						fprintf(stderr,"mode error: invalid command. invalid mode parameter specified\n");
					}
				} else {
					fprintf(stderr,"mode error: invalid command. mode does not take more than one parameter\n");
				}
			}

			// handle exit command
			else if(strcmp(cmdtoks[i][0],"exit")==0) {
				if (cmdtoks[i][1] == NULL) {
					exit=1;
					if (!par)
						break;
				} else {
					fprintf(stderr,"exit error: invalid command. exit does not take any parameters\n");
				}
			}

			// handle commands
			else {
				pid = fork();
				p_running++;

				if (pid==0) {
					int skip=0;
					tmp = head;
					while (tmp!=NULL) {
						int size = strlen(tmp->name)+strlen(cmdtoks[i][0]);
						char *path = (char*) malloc(size);
						strcpy(path, tmp->name);
						strcat(path, "/");
						strcat(path, cmdtoks[i][0]);

						if (execv(path, cmdtoks[i]) != -1){
							skip=1;
							break;
						}

						free(path);
						tmp = tmp->next;
					}
					if (!skip) {
						if (execv(cmdtoks[i][0], cmdtoks[i]) < 0) {
							fprintf(stderr, "execv failed: %s\n", strerror(errno));
						}
					}
					return 0;
				} else {
					if (!par) {
						// wait for processes to finish in sequential mode
                                                waitpid(pid, &childrv, 0);
						p_running--;
					}
				}
			}

			i++;
		}

		if (par) {
			// wait for processes to finish in parallel mode
			while (p_running > 0) {
				waitpid(-1, &childrv, 0);
				p_running--;
			}
		}

		// free heap memory
		i=0; j=0;
		while (cmdtoks[i]!=NULL) {
			while (cmdtoks[i][j]!=NULL) {
				free(cmdtoks[i][j]);
				j++;
			}
			free(cmdtoks[i]);
			i++;
		}
		free(cmdtoks);

		if (exit)
			break;

		printf("$$ ");
		fflush(stdout);

	}

	// free more heap memory
	while (head!=NULL) {
		tmp = head;
		head = head->next;
		free(tmp);
	}

    	return 0;
}

