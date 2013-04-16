#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>

struct list_element {
	pid_t pid;
	struct list_element * next;
};

typedef struct list_element item;

void cut_characters (char * string, int num){
	int len = strlen (string);
	string += (len-num);
	*string = '\0';
}

int main(int argc, char const *argv[])
{
	char * bash = "smallshell:";
	char * cwd = getcwd (0, 0);
	char * exit_string = "exit";
	char * cd_string = "cd";
	int active_pids = 0;
	int i = 0;
	int retval;
	char input[70];
	int status;
	pid_t pid;
	item * curr, * head;
	head = NULL;
	while (1){
		printf("%s%s$ ", bash, cwd);
		fgets (input, 400, stdin);
		cut_characters (input, 1);

		item * current = curr;
		for (i = 0; i <active_pids; i++)
		{
			pid = waitpid (current->pid, &status, WNOHANG);
			if (pid > 0){
				printf ("Process with PID %d has exited.\n", pid);
				active_pids--;
			}
			if (pid == -1){
				//error
			}
		current = current->next;
		}
		char *program_args[20];
		char * token;
		char * input_string;
		input_string = strdup (input);
		if (input_string == NULL){
			continue;
		}
		
		int background = input_string[strlen (input_string)-1] == '&';
		if (background)
			cut_characters (input_string, 2);
		while ((token = strsep(&input_string, " ")) != NULL)
		{
			if (!strcmp (token, exit_string)){
				printf("Exiting...\n");
				exit (0);
			}
			if (!strcmp (token, cd_string)){
				token = strsep(&input_string, " ");
				struct stat dir;
				int exists = stat (token, &dir);
				if (exists == -1 || !S_ISDIR (dir.st_mode)){
					printf("directory does not exist, defaulting to homedir\n");
					token = getenv ("HOME");
				}
				//cd to token
				chdir (token);
				cwd = getcwd (0, 0);
			}
			program_args[i] = strdup (token);

			i = i+1;
		}
		program_args[i] = NULL;
		free(input_string);
		
		pid = fork ();
		if (pid == 0){
			printf ("ARGS: %s\n", *program_args);
			retval = execvp (*program_args, program_args);
			if (retval != -1)
				printf ("Process with PID %d created\n", pid);
		}
		
		if (!background){
			waitpid (pid, &status, 0);
		} else{
			curr = (item *)malloc (sizeof (item));
			curr->pid = pid;
			curr->next = head;
			head = curr;
			active_pids++;
		}

		printf("Active child background processes: %d\n",active_pids);

	}

}
