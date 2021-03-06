#include <stdio.h>
#include <sock.h>

#define DAEMON_PORT "1950"

int main(int argc,
	char ** argv) {

	printf("Starting server daemon: ");
	if (startListenSocket(DAEMON_PORT))
	{
		printf("OK.\n");
	}
	else
	{
		printf("FAILED. PRESS ENTER KEY TO ABORT.\n");
		(void)getchar();
		return 1;
	}

	(void)getchar();

	printf("Stopping server daemon: ");
	if (closeListenSocket() == 0)
		printf("OK.\n");
	else
		printf("FAILED.\n");
		
	printf("Exited. Press enter key to close window. ");
	(void)getchar();

	return 0;
}