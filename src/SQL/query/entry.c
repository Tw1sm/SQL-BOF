#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void CustomQuery(char* server, char* user, char* password, char* database, char* link, char* impersonate, char* query)
{
    SQLHENV env		= NULL;
    SQLHSTMT stmt 	= NULL;
	SQLHDBC dbc 	= NULL;
	SQLRETURN ret;


    if (link == NULL)
	{
		dbc = ConnectToSqlServer(&env, server, user, password, database);
	}
	else
	{
		dbc = ConnectToSqlServer(&env, server, user, password, NULL);
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
	ret = ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	if (!SQL_SUCCEEDED(ret))
	{
		internal_printf("[!] Error allocating statement handle\n");
		goto END;
	}

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
	char* user;
	char* password;
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
	user	 	= BeaconDataExtract(&parser, NULL);
	password 	= BeaconDataExtract(&parser, NULL);
	database 	= BeaconDataExtract(&parser, NULL);
	link 		= BeaconDataExtract(&parser, NULL);
	impersonate = BeaconDataExtract(&parser, NULL);
	query 		= BeaconDataExtract(&parser, NULL);

	server = *server == 0 ? "localhost" : server;
	user = *user == 0 ? NULL : user;
	password = *password == 0 ? NULL : password;
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
	
	CustomQuery(server, user, password, database, link, impersonate, query);

	printoutput(TRUE);
};

#else

int main()
{
	internal_printf("============ BASE TEST ============\n\n");
	CustomQuery("castelblack.north.sevenkingdoms.local", NULL, NULL, "master", NULL, NULL, "SELECT name, database_id FROM sys.databases;");

	internal_printf("\n\n============ IMPERSONATE TEST ============\n\n");
	CustomQuery("castelblack.north.sevenkingdoms.local", NULL, NULL, "master", NULL, "sa", "SELECT name, database_id FROM sys.databases;");

	internal_printf("\n\n============ LINK TEST ============\n\n");
	CustomQuery("castelblack.north.sevenkingdoms.local", NULL, NULL, "master", "BRAAVOS", NULL, "SELECT name, database_id FROM sys.databases;");
}

#endif
