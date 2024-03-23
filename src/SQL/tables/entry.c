#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void CheckTables(char* server, char* database, char* link, char* impersonate)
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
		internal_printf("[*] Enumerating tables in the %s database on %s\n\n", database, server);
	}
	else
	{
		internal_printf("[*] Enumerating tables in the %s database on %s via %s\n\n", database, link, server);
	}
	

	//
	// allocate statement handle
	//
	ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

	//
	// Construct query
	//
	char* prefix = "SELECT * FROM ";
	char* suffix = ".INFORMATION_SCHEMA.TABLES;";
	
	char* query = (char*)intAlloc((MSVCRT$strlen(prefix) + MSVCRT$strlen(database) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
	
	MSVCRT$strcpy(query, prefix);
	MSVCRT$strcat(query, database);
	MSVCRT$strcat(query, suffix);

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

	//
	// parse beacon args 
	//
	datap parser;
	BeaconDataParse(&parser, Buffer, Length);
	
	server	 	= BeaconDataExtract(&parser, NULL);
	database 	= BeaconDataExtract(&parser, NULL);
	link 		= BeaconDataExtract(&parser, NULL);
	impersonate = BeaconDataExtract(&parser, NULL);

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
	
	CheckTables(server, database, link, impersonate);

	printoutput(TRUE);
};

#else

int main()
{
	internal_printf("============ BASE TEST ============\n\n");
	CheckTables("castelblack.north.sevenkingdoms.local", "master", NULL, NULL);

	internal_printf("\n\n============ IMPERSONATE TEST ============\n\n");
	CheckTables("castelblack.north.sevenkingdoms.local", "master", NULL, "sa");

	internal_printf("\n\n============ LINK TEST ============\n\n");
	CheckTables("castelblack.north.sevenkingdoms.local", "master", "BRAAVOS", NULL);
}

#endif
