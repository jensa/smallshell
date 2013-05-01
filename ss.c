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
#include <time.h>
#include <sys/time.h>
#include <signal.h>

struct list_element {
	pid_t pid;
	double start_time;
	struct list_element * next;
};

typedef struct list_element item;

/*Global variables */
item * head;

/* Function declarations */
double read_timer ();
void remove_item (item *);
void cut_characters (char *, int);
void print_statistics (item *);
void add_item (item *);

void sigint_handler (int sig){
	//Do nothing...
}

/* Timer function. Uses gettimeofday (2) to get the current time with high precision. 
	Returns a double representing the current time in milli(??)seconds */ 
double read_timer() {
  static bool initialized = false;
  static struct timeval start;
  struct timeval end;
  if( !initialized )
  {
    gettimeofday( &start, NULL );
    initialized = true;
  }
  gettimeofday( &end, NULL );
  return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

void remove_item (item * node){
	item * cur = head;
	if (node == head){
		head = node->next;
		free (node);
		return;
	}
	while (cur->next != NULL && cur->next != node){
		printf ("GETTING NEXT IN REMOVE\n");
		cur = cur->next;
	}
	cur->next = node->next;
	free (node);
}

void add_item (item * node){
	item * cur = head;	
	while (cur->next != NULL){
		printf ("GETTING NEXT IN ADD\n");
		cur = cur->next;
	}
	cur->next = node;
}

void cut_characters (char * string, int num){
	int len = strlen (string);
	string += (len-num);
	*string = '\0';
}

void print_statistics (item * process){
	double current_time = read_timer ();
	printf ("The process with PID %d terminated after running for %f seconds\n", process->pid, (current_time - process->start_time));
}

int main(int argc, char const *argv[])
{
	signal (SIGINT, sigint_handler);

	char * bash = "smallshell:";
	char * current_working_directory = getcwd (0, 0);
	char * exit_string = "exit";
	char * cd_string = "cd";
	int active_pids = 0;
	int i = 0; /* loop variable */
	int retval, status;
	char input[70];

	pid_t pid;


	/* main program loop, takes input from user and executes its commands */
	while (1){
		printf("%s%s$ ", bash, current_working_directory); /* print out the bash symbol for taking input */
		fgets (input, 400, stdin); /* read one line of input from STDIN */
		cut_characters (input, 1); /* cut the newline character from the end of the input string */
		item * current = head;
		/* Go through the linked list of all active background processes 
		and check for status change (killed/exited processes) */
		int active_pid_loop = active_pids;
		for (i = 0; i <active_pid_loop; i++){
			pid = waitpid (current->pid, &status, WNOHANG); /* check process for status change. 
			WNOHANG option ensures the call returns immediately. */
			if (pid > 0){
				//Check status variable after this to see if the process ended normally
				//
				print_statistics (current); // Print stats (TA BORT)
				item * temp_current = current->next; // temporarily store next item in list
				remove_item (current); // remove current item from list
				current = temp_current; // set current element to next item in list
				active_pids--;
			}
			else if (pid == -1){
				//error
			} 
			else{
				current = current->next;
			}
		}
		char *program_args[20]; /* Array holding program arguments */
		char * token;
		char * input_string;
		input_string = strdup (input);
		/*check for empty input and continue loop if it was empty */
		if (input_string == NULL || strlen (input_string) == 0){
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
				token = strsep(&input_string, " "); /* get next token on string, which should contain the path to cd to */
				printf ("path: %s\n", token);
				struct stat dir;
				int exists = stat (token, &dir); /* Checks if the path given exists, and if it is a dir */
				if (exists == -1 || !S_ISDIR (dir.st_mode)){
					printf("directory does not exist, defaulting to homedir\n");
					token = getenv ("HOME");
				}
				chdir (token); /* change working directory to the given path */
				current_working_directory = getcwd (0, 0); /* modify the bash symbol to reflect the new path */
				input_string = NULL; /* Empty the input string (it might contain garabage efter the path) */
				break;
			}
			program_args[i] = strdup (token);
			i++;
		}
		program_args[i] = NULL;
		free(input_string);

		pid = fork ();
		if (pid == 0){
			retval = execvp (*program_args, program_args);
			if (retval != -1)
				printf ("Process with PID %d created\n", pid);
		}
		
		if (!background){
			double start_time = read_timer ();
			waitpid (pid, &status, 0);
			double run_time = read_timer () - start_time;
			printf ("The process with PID %d terminated after running for %f seconds\n", pid, run_time);
		} else{
			// Add the background process PID to the linked list 
			item * node = (item *)malloc (sizeof (item));
			node->pid = pid;
			node->start_time = read_timer ();
			if (head == NULL){
				head = node;
			} else{
			add_item (node);
			}
			active_pids++;
		}
	}

}
