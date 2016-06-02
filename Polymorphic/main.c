#include <database.h>
#include <stdio.h>
#include <sock.h>

#define DAEMON_PORT "1950"

int main(int argc,
	char ** argv) {

	sqlite3* db;
	printf("Opening SQLite database: ");
	if (openDatabase(&db))
	{
		printf("OK.\n");
	}
	else
	{
		printf("FAILED. PRESS ENTER KEY TO ABORT.\n");
		(void)getchar();
		return 1;
	}

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



	(void)getchar();

	printf("Stopping server daemon: ");
	if (closeSocketLib() == 0)
		printf("OK.\n");
	else
		printf("FAILED.\n");

	printf("Closing SQLite Database: ");
	if (closeDatabase(db) == SQLITE_OK)
		printf("OK.\n");
	else
		printf("FAILED.\n");

	printf("Exited. Press enter key to close window. ");
	(void)getchar();

	return 0;
}