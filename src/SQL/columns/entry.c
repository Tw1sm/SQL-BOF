#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void CheckTableColumns(char* server, char* database, char* link, char* impersonate, char* table)
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
		internal_printf("[*] Displaying columns from table %s in %s on %s\n\n", table, database, server);
	}
	else
	{
		internal_printf("[*] Displaying columns from table %s in %s on %s via %s\n\n", table, database, link, server);
	}
	

	//
	// allocate statement handle
	//
	ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

	if (link == NULL)
	{

		//
		// Construct USE database statement
		//
		char* dbPrefix = "USE ";
		char* dbSuffix = ";";
		char* useStmt = (char*)intAlloc((MSVCRT$strlen(dbPrefix) + MSVCRT$strlen(database) + MSVCRT$strlen(dbSuffix) + 1) * sizeof(char));
		MSVCRT$strcpy(useStmt, dbPrefix);
		MSVCRT$strcat(useStmt, database);
		MSVCRT$strcat(useStmt, dbSuffix);

		if (!HandleQuery(stmt, (SQLCHAR*)useStmt, link, impersonate, TRUE)){
			goto END;
		}

		//
		// Construct query
		//
		char* tablePrefix = "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
							"WHERE TABLE_NAME = '";
		char* tableSuffix = "' ORDER BY ORDINAL_POSITION;";
		
		char* query = (char*)intAlloc((MSVCRT$strlen(tablePrefix) + MSVCRT$strlen(table) + MSVCRT$strlen(tableSuffix) + 1) * sizeof(char));
		
		MSVCRT$strcat(query, tablePrefix);
		MSVCRT$strcat(query, table);
		MSVCRT$strcat(query, tableSuffix);

		BeaconPrintf(CALLBACK_OUTPUT, "Query: %s\n", query);

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
		//
		// Construct query
		//
		char* dbPrefix = "SELECT COLUMN_NAME FROM ";
		char* tablePrefix = ".INFORMATION_SCHEMA.COLUMNS "
							"WHERE TABLE_NAME = '";
		char* tableSuffix = "' ORDER BY ORDINAL_POSITION;";
		
		char* query = (char*)intAlloc((MSVCRT$strlen(dbPrefix) + MSVCRT$strlen(database) + MSVCRT$strlen(tablePrefix) + MSVCRT$strlen(table) + MSVCRT$strlen(tableSuffix) + 1) * sizeof(char));

		MSVCRT$strcpy(query, dbPrefix);
		MSVCRT$strcat(query, database);
		MSVCRT$strcat(query, tablePrefix);
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
	
	CheckTableColumns(server, database, link, impersonate, table);

	printoutput(TRUE);
};

#else

int main()
{
	CheckTableColumns("192.168.0.215", "master", NULL, NULL, "CustomTable");
}

#endif
