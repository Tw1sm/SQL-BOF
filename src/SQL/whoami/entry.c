#include <windows.h>
#include "bofdefs.h"
#include "base.c"
#include "sql.c"

void PrintMemberStatus(char* roleName, char* status) {
	if (status[0] == '0') {
		internal_printf(" |--> User is NOT a member of the %s role\n", roleName);
	}
	else {
		internal_printf(" |--> User is a member of the %s role\n", roleName);
	}
}

void Whoami(char* server, char* database) {
    SQLHENV env		= NULL;
    SQLHSTMT stmt 	= NULL;

	//
	// default server roles
	//
	char* roles[] = { 
		"sysadmin",
		"setupadmin", 
		"serveradmin", 
		"securityadmin",
        "processadmin",
		"diskadmin",
		"dbcreator",
		"bulkadmin"
	};
	
	
	char* sysUser 		= NULL;
	char* mappedUser 	= NULL;
	char** dbRoles 		= NULL;

    SQLHDBC dbc = ConnectToSqlServer(&env, server, database);

    if (dbc == NULL) {
		goto END;
	}

	internal_printf("[*] Determining user permissions on %s\n", server);

	//
	// allocate statement handle
	//
	ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

	//
	// first query
	//
	SQLCHAR* query = (SQLCHAR*)"SELECT SYSTEM_USER;";
	ExecuteQuery(stmt, query);
	sysUser = GetSingleResult(stmt, FALSE);
	internal_printf("[*] Logged in as %s\n", sysUser);

	//
	// close the cursor
	//
	ODBC32$SQLCloseCursor(stmt);

	//
	// second query
	//
	query = (SQLCHAR*)"SELECT USER_NAME();";
	ExecuteQuery(stmt, query);
	mappedUser = GetSingleResult(stmt, FALSE);
	internal_printf("[*] Mapped to the user %s\n", mappedUser);

	//
	// close the cursor
	//
	ODBC32$SQLCloseCursor(stmt);

	//
	// third query
	//
	internal_printf("[*] Gathering roles...\n");
	query = (SQLCHAR*)"SELECT [name] from sysusers where issqlrole = 1;";
	ExecuteQuery(stmt, query);
	dbRoles = GetMultipleResults(stmt, FALSE);
	
	//
	// close the cursor
	//
	ODBC32$SQLCloseCursor(stmt);
	
	//
	// fourth query (loop)
	//
	for (int i = 0; dbRoles[i] != NULL; i++) {
		char* role = dbRoles[i];
		char query[1024];
		MSVCRT$sprintf(query, "SELECT IS_MEMBER('%s');", role);
		ExecuteQuery(stmt, query);
		PrintMemberStatus(role, GetSingleResult(stmt, FALSE));
		ODBC32$SQLCloseCursor(stmt);
	}
	
	//
	// fifth query (loop)
	//
	for (int i = 0; i < sizeof(roles) / sizeof(roles[0]); i++) {
		char* role = roles[i];
		char query[1024];
		MSVCRT$sprintf(query, "SELECT IS_SRVROLEMEMBER('%s');", role);
		ExecuteQuery(stmt, query);
		PrintMemberStatus(role, GetSingleResult(stmt, FALSE));
		ODBC32$SQLCloseCursor(stmt);
	}

END:
	DisconnectSqlServer(env, dbc, stmt);
}


#ifdef BOF
VOID go( 
	IN PCHAR Buffer, 
	IN ULONG Length 
) 
{
	char* server 	= NULL;
	char* database 	= NULL;

	//
	// parse beacon args 
	//
	datap parser;
	BeaconDataParse(&parser, Buffer, Length);
	server = BeaconDataExtract(&parser, NULL);
	database = BeaconDataExtract(&parser, NULL);

	if (database == NULL) {
		database = "master";
	}

	if (server == NULL) {
		server = "localhost";
	}

	if(!bofstart())
	{
		return;
	}
	
	Whoami(server, database);

	printoutput(TRUE);
};

#else

int main()
{
	Whoami("192.168.0.215", "master");
}

#endif
