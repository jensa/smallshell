/*
 *
 * NAME:
 *   smallshell  -  this program is a simple shell 
 * 
 * SYNTAX:
 *   command [-arguments]
 *
 * DESCRIPTION:
 *   The smallshell reads the users commands and executes them if possible. The shell can handle
 *   background processes and two commands are built in; cd and exit. The user can also give arguments to the 
 *   commands given. When executing and terminating processes, statistics of the process id and execution time for 
 *   foreground processes will be printed. 
 *
 *   
 * EXAMPLES:
 *   ./test 4
 *
 */
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

/*
* Struct representing elements in a linked list. Each element has got a process id
* and a pointer to the next element. 
*/
struct list_element {
	pid_t pid;
	struct list_element * next;
};

typedef struct list_element item;

/*Global variables */
item * head;

/* Function declarations */
double read_timer ();
void remove_item (item *);
void cut_characters (char *, int);
void add_item (item *);
void sigint_handler (int);
void clean_exit (int);

/* Function definitions */

/* sigint_handler
 *
 * This function is registered as the handler of the SIGINT signal for the program's main process.
 * The function simply ignores the signal, as the program should not exit upon recieving a SIGINT.
 */
void sigint_handler (int sig){
	//Do nothing when recieveing this signal.
}
/* clean_exit
 *
 * Kills all active background processes of the main program using kill(2), and exits with status code 0.
 */
void clean_exit (int active_pids){
	int active_pid_loop = active_pids;
	int i;
	item * current = head;
	for (i = 0; i <active_pid_loop; i++){
		kill (current->pid, SIGKILL); /* kill the active background process */
		current = current->next;
	}
	exit (0);
}

/* read_timer
* 
* Timer function. Uses gettimeofday (2) to get the current time with high precision. 
* Returns a double representing the current time in milliseconds. 
*/ 
double read_timer() {
  struct timeval time_struct;
  gettimeofday( &time_struct, NULL );
  return time_struct.tv_sec + 1.0e-6 * time_struct.tv_usec;
}

/* check_background_processes
* 
* check_background_processes takes an integer representing the number of active processes
* and returns the new number of active processes. 
* 
* It loops through the linked list and checks if the different processes have been completed.
* If the process has been completed, statistics will be printed and the element representing 
* the process will be removed from the linked list. 
*/
int check_background_processes (int active_pids){
	int active_pid_loop = active_pids;
	int i, status;
	pid_t pid;
	item * current = head;
	for (i = 0; i <active_pid_loop; i++){
			pid = waitpid (current->pid, &status, WNOHANG); /* check process for status change. 
			WNOHANG option ensures the call returns immediately. */
			if (pid > 0){
				if (status != 0){
					printf ("The background process with PID %d exited with status code %d\n", current->pid, status);
				} else{
					printf ("The background process with PID %d completed normally\n", current->pid);
				}
				item * temp_current = current->next; // temporarily store next item in list
				remove_item (current); // remove current item from list
				current = temp_current; // set current element to next item in list
				active_pids--;
			}
			else if (pid == -1){
				printf ("The background process with PID %d exited abnormally with status code %d\n", current->pid, status);
			} 
			else{
				current = current->next;
			}
		}
	return active_pids;
}

/* remove_item
* 
* Remove_item takes a node, deletes it from the linked list and frees the memory. 
* The next-pointer from the node pointing at the deleted node is changed to the node after the deleted one.
*/
void remove_item (item * node){
	item * cur = head;
	if (node == head){
		head = node->next;
		free (node);
		return;
	}
	while (cur->next != NULL && cur->next != node){
		cur = cur->next;
	}
	cur->next = node->next;
	free (node);
}

/* add_item
* 
* Add_item takes a new node and adds it to the linked list. The last elements next-pointer will now be 
* pointing at the new node.
*/
void add_item (item * node){
	item * cur = head;	
	while (cur->next != NULL){
		cur = cur->next;
	}
	cur->next = node;
}

/* cut_characters
* 
* Cut_charachters takes a string and an integers. The number given represents how many charachters
* will be cut from the given string. 
*/
void cut_characters (char * string, int num){
	int len = strlen (string);
	string += (len-num);
	*string = '\0';
}

int main(int argc, char const *argv[])
{
	signal (SIGINT, sigint_handler);

	char * bash = "smallshell:";
	char * current_working_directory = getcwd (0, 0);
	char * exit_string = "exit";
	char * cd_string = "cd";
	int active_pids = 0;
	int retval, status;
	char input[70];

	pid_t pid;


	/* main program loop, takes input from user and executes its commands */
	while (true){
		printf("%s%s$ ", bash, current_working_directory); /* print out the bash symbol for taking input */
		fgets (input, 70, stdin); /* read one line of input from STDIN */
		
		/* Go through the linked list of all active background processes 
		and check for status change (killed/exited processes) */
		active_pids = check_background_processes (active_pids);
		cut_characters (input, 1); /* cut the newline character from the end of the input string */
		char *program_args[20]; /* Array holding program arguments */
		char * token;
		char * input_string = strdup (input);
		/*check for empty input and continue loop if it was empty */
		if (input_string == NULL || strlen (input_string) == 0){
			continue;
		}
		bool background = input_string[strlen (input_string)-1] == '&'; 
		/* background is used to determine if the user wants to run a background process*/
		if (background)
			cut_characters (input_string, 2);
		token = strsep(&input_string, " ");
		/*Checks if the given command is exit, and if it is, the shell will exit */
		if (!strcmp (token, exit_string)){
			printf("Exiting...\n");
			clean_exit (active_pids);
		}
		/*Checks if the given command is cd */
		else if (!strcmp (token, cd_string)){
			/* get next token on string, which should contain the path to cd to */
			token = strsep(&input_string, " "); 
			struct stat dir;
			int exists = stat (token, &dir); 
			/* Checks if the path given exists, and if it is a dir */
			if (exists == -1 || !S_ISDIR (dir.st_mode)){
				printf("directory does not exist, defaulting to homedir\n");
				token = getenv ("HOME"); /* get HOME variable */
				if (token == NULL)
					printf ("HOME environment variable not found\n");
			}
			retval = chdir (token); /* change working directory to the given path */
			if (retval == -1)
				printf ("Unable to change directory, errno was: %d\n", errno);
			else
				current_working_directory = getcwd (0, 0); /* modify the bash symbol to reflect the new path */
			continue;
		}
		int i=0;
		/*This while loop separates the given arguments*/
		do{
			program_args[i] = strdup (token);
			i++;
		}while ((token = strsep(&input_string, " ")) != NULL);
		free(input_string);
		program_args[i] = '\0';
		/* A new process is created and the given command will be executed using execvp*/
		pid = fork ();
		if( -1 == pid ){ /* fork() failed */
			printf ( "Cannot fork()\n" );
		}
		if (pid == 0){
			retval = execvp (*program_args, program_args);
			if (retval == -1){
				printf ("An error occurred while executing %s\n", *program_args);
				exit (0);
			}
		}
		
		if (!background){
			/*The foreground process will be waited for to terminate and the time it took is measured */
			printf ("Process with PID %d created\n", pid);
			double start_time = read_timer ();
			retval = waitpid (pid, &status, 0);
			double run_time = read_timer () - start_time;
			if (retval == -1){
				printf ("The process with PID %d exited abnormally with status code %d\n", pid, status);
			} else{
				if (status != 0){
					printf ("The process with PID %d exited after running for %f seconds with status code %d\n", pid, run_time, status);
				} else{
					printf ("The process with PID %d terminated after running for %f seconds\n", pid, run_time);
				}
			}
		} else{
			/*The background process's PID is added to the linked list */
			printf ("Background process with PID %d created\n", pid);
			item * node = (item *)malloc (sizeof (item));
			node->pid = pid;
			if (head == NULL){
				head = node;
			} else{
			add_item (node);
			}
			active_pids++;
		}
	}
}
