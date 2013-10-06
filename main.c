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

int main(int argc, char **argv) {
	char prompt[3] = "$$ ";
	char buffer[1024];
	int i;
	int cmds = 0;
	char ***cmdtoks;

	printf("%s", prompt);
	fflush(stdout);

	while (fgets(buffer, 1024, stdin) != NULL) {
		fgets(buffer, 1024, stdin);
		// 1. count the number of commands and locate any comments
		cmds = 1;
		i=0;
		for (; i<1024; i++) {
			if ( buffer[i] == (int)';' )
				cmds++;
			if ( buffer[i] == (int)'#' ) {
				buffer[i]='\0';
				break;
			}
		}

		// 2. tokenize commands and organize into array of array of pointers
		cmdtoks = tokenify(&buffer, cmds);
		i=0;
		for (; i < cmds; i++) {
			printf("Command %i:\n", i);
			int j=0;
			while ( cmdtoks[i][j] != NULL ) {
				printf("%s\n", cmdtoks[i][j]);
				j++;
			}
		}

		// 3. begin executing commands




		printf("%s",prompt);
		fflush(stdout);

	}

	i=0;
	free(cmdtoks);
    	return 0;
}

