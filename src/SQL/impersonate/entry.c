#include "bofdefs.h"
#include "base.c"
#include "sql.c"


void CheckImpersonate(char* server, char* user, char* password, char* database)
{
    SQLHENV env		= NULL;
    SQLHSTMT stmt 	= NULL;
	SQLRETURN ret;


    SQLHDBC dbc = ConnectToSqlServer(&env, server, user, password, database);

    if (dbc == NULL) 
	{
		goto END;
	}

	internal_printf("[*] Enumerating users that can be impersonated on %s\n\n", server);

	//
	// allocate statement handle
	//
	ret = ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	if (!SQL_SUCCEEDED(ret)) {
		internal_printf("[!] Error allocating statement handle\n");
		goto END;
	}
	
	//
	// Run the query
	//
	SQLCHAR* query = (SQLCHAR*)"SELECT distinct b.name FROM sys.server_permissions a "
            		"INNER JOIN sys.server_principals b ON a.grantor_principal_id "
            		"= b.principal_id WHERE a.permission_name = 'IMPERSONATE';";
	if (!ExecuteQuery(stmt, query))
	{
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
	char* server 	= NULL;
	char* user	= NULL;
	char* password	= NULL;
	char* database 	= NULL;

	//
	// parse beacon args 
	//
	datap parser;
	BeaconDataParse(&parser, Buffer, Length);

	server 		= BeaconDataExtract(&parser, NULL);
	user	 	= BeaconDataExtract(&parser, NULL);
	password 	= BeaconDataExtract(&parser, NULL);
	database 	= BeaconDataExtract(&parser, NULL);

	server = *server == 0 ? "localhost" : server;
	user = *user == 0 ? NULL : user;
	password = *password == 0 ? NULL : password;
	database = *database == 0 ? "master" : database;

	if(!bofstart())
	{
		return;
	}
	
	CheckImpersonate(server, user, password, database);

	printoutput(TRUE);
};

#else

int main()
{
	internal_printf("============ BASE TEST ============\n\n");
	CheckImpersonate("castelblack.north.sevenkingdoms.local", NULL, NULL, "master");
}

#endif
