#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void CustomQuery(char* server, char* database, char* link, char* impersonate, char* query)
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

	if (link == NULL)
	{
		internal_printf("[*] Executing custom query on %s\n\n", server);
	}
	else
	{
		internal_printf("[*] Executing custom query on %s via %s\n\n", link, server);
	}
	

	//
	// allocate statement handle
	//
	ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

	//
	// Run the query
	//
	if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, FALSE)){
		goto END;
	}
	PrintQueryResults(stmt, TRUE);

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
	char* query;

	//
	// parse beacon args 
	//
	datap parser;
	BeaconDataParse(&parser, Buffer, Length);
	
	server	 	= BeaconDataExtract(&parser, NULL);
	database 	= BeaconDataExtract(&parser, NULL);
	link 		= BeaconDataExtract(&parser, NULL);
	impersonate = BeaconDataExtract(&parser, NULL);
	query 		= BeaconDataExtract(&parser, NULL);

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

	if (query == NULL)
	{
		return;
	}
	
	CustomQuery(server, database, link, impersonate, query);

	printoutput(TRUE);
};

#else

int main()
{
	CustomQuery("192.168.0.215", "master", NULL, NULL, "SELECT TOP 5 * FROM sys.databases;");
}

#endif
