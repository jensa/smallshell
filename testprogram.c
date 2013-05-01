#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
	int sleep_time = 5;
	if (argc > 1)
		sleep_time = atoi (argv[1]);
	sleep (sleep_time);
	printf("testprogram complete, ran for %d seconds\n", sleep_time);
	exit (0);
}
