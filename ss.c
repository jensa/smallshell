#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

int main(int argc, char const *argv[])
{
	char * bash = "tjenna> ";
	char input[400];
	while (1){
		printf("%s", bash);
		fgets (input, 400, stdin);
		int len = strlen (input);
		input[len - 1] = '\0';
		char *program_args[20];
		char * token;
		char * input_string;
		input_string = strdup (input);
		if (input_string == NULL)
			continue;
		int i = 0;
		while ((token = strsep(&input_string, " ")) != NULL)
		{
			program_args[i] = strdup (token);
			i = i+1;
		}
		program_args[i] = NULL;
		free(input_string);
		int pid, status;
		pid = fork ();
		if (pid == 0){
			execvp (*program_args, program_args);
		}
		waitpid (pid, &status, 0);
	}

}