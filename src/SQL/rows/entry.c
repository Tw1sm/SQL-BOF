#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void CheckTableRows(char* server, char* database, char* link, char* impersonate, char* table)
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
		internal_printf("[*] Getting row count from table %s in %s on %s\n\n", table, database, server);
	}
	else
	{
		internal_printf("[*] Getting row count from table %s in %s on %s via %s\n\n", table, database, link, server);
	}
	

	//
	// allocate statement handle
	//
	ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

	// TODO: use linked RPC query

	//
	// Construct USE database statement
	//
	char* dbPrefix = "USE ";
	char* dbSuffix = "; ";
	char* tablePrefix = "SELECT COUNT(*) as row_count FROM ";
	char* tableSuffix = ";";

	if (link == NULL)
	{
		//
		// Not using link; need to execute two queries
		//
		char* useStmt = (char*)intAlloc((MSVCRT$strlen(dbPrefix) + MSVCRT$strlen(database) + MSVCRT$strlen(dbSuffix) + 1) * sizeof(char));
		MSVCRT$strcpy(useStmt, dbPrefix);
		MSVCRT$strcat(useStmt, database);
		MSVCRT$strcat(useStmt, dbSuffix);

		if (!HandleQuery(stmt, (SQLCHAR*)useStmt, link, impersonate, TRUE)){
			goto END;
		}

		//
		// close the cursor
		//
		ODBC32$SQLCloseCursor(stmt);

		//
		// Construct query
		//
		char* query = (char*)intAlloc((MSVCRT$strlen(tablePrefix) + MSVCRT$strlen(table) + MSVCRT$strlen(tableSuffix) + 1) * sizeof(char));
		
		MSVCRT$strcpy(query, tablePrefix);
		MSVCRT$strcat(query, table);
		MSVCRT$strcat(query, tableSuffix);

		//
		// Run the query
		//
		if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, TRUE)){
			goto END;
		}
		PrintQueryResults(stmt, TRUE);
	}
	else
	{
		char* sep = ".";

		//
		// linked RPC query funkiness, idk what to do so lets get the table schema
		//
		if (!GetTableShema(stmt, link, database, table)){
			internal_printf("[!] Failed to get table schema for %s\n", table);
			goto END;
		}

		char* schema = GetSingleResult(stmt, FALSE);
		internal_printf("[*] Table schema for %s is: %s\n\n", table, schema);

		//
		// Close the cursor
		//
		ODBC32$SQLCloseCursor(stmt);

		//
		// Prep statement for linked RPC query
		// tableprefix + database + sep + schema + sep + table + suffix
		//
		char* query = (char*)intAlloc((MSVCRT$strlen(tablePrefix) + MSVCRT$strlen(database) + MSVCRT$strlen(sep) + MSVCRT$strlen(schema) + MSVCRT$strlen(sep) + MSVCRT$strlen(table) + MSVCRT$strlen(tableSuffix) + 1) * sizeof(char));
		MSVCRT$strcpy(query, tablePrefix);
		MSVCRT$strcat(query, database);
		MSVCRT$strcat(query, sep);
		MSVCRT$strcat(query, schema);
		MSVCRT$strcat(query, sep);
		MSVCRT$strcat(query, table);

		if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, TRUE)){
			goto END;
		}
		PrintQueryResults(stmt, TRUE);
	}

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
	char* table;
	char* link;
	char* impersonate;

	//
	// parse beacon args 
	//
	datap parser;
	BeaconDataParse(&parser, Buffer, Length);
	
	server	 	= BeaconDataExtract(&parser, NULL);
	database 	= BeaconDataExtract(&parser, NULL);
	table 		= BeaconDataExtract(&parser, NULL);
	link 		= BeaconDataExtract(&parser, NULL);
	impersonate = BeaconDataExtract(&parser, NULL);


	server = *server == 0 ? "localhost" : server;
	database = *database == 0 ? "master" : database;
	table = *table == 0 ? NULL : table;
	link = *link  == 0 ? NULL : link;
	impersonate = *impersonate == 0 ?  NULL : impersonate;

	if(!bofstart())
	{
		return;
	}

	if (table == NULL)
	{
		internal_printf("[!] Table argument is required\n");
		printoutput(TRUE);
		return;
	}

	if (UsingLinkAndImpersonate(link, impersonate))
	{
		return;
	}
	
	CheckTableRows(server, database, link, impersonate, table);

	printoutput(TRUE);
};

#else

int main()
{
	internal_printf("============ BASE TEST ============\n\n");
	CheckTableRows("castelblack.north.sevenkingdoms.local", "master", NULL, NULL, "spt_monitor");

	internal_printf("\n\n============ IMPERSONATE TEST ============\n\n");
	CheckTableRows("castelblack.north.sevenkingdoms.local", "master", NULL, "sa", "spt_monitor");

	internal_printf("\n\n============ LINK TEST ============\n\n");
	CheckTableRows("castelblack.north.sevenkingdoms.local", "master", "BRAAVOS", NULL, "spt_monitor");
}

#endif
