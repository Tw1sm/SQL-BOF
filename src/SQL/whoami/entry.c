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

void Whoami() {
    SQLHENV env = NULL;
    SQLHSTMT stmt = NULL;

	/*  define the default roles well check membership for  */
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
	
	/*  query output vars  */
	char* sysUser = NULL;
	char* mappedUser = NULL;
	char** dbRoles = NULL;
	
    char* server = "oxenfurt.redania.local";
    char* dbName = "master";

    SQLHDBC dbc = ConnectToSqlServer(&env, server, dbName);

    if (dbc == NULL) {
		goto END;
	}

	internal_printf("[*] Determining user permissions on %s\n", server);

	/*  Allocate a statement handle  */
	ODBC32$SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

	/*  first query  */
	SQLCHAR* query = (SQLCHAR*)"SELECT SYSTEM_USER;";
	ExecuteQuery(stmt, query);
	sysUser = GetSingleResult(stmt, FALSE);
	internal_printf("[*] Logged in as %s\n", sysUser);

	/*  Close the cursor  */
	ODBC32$SQLCloseCursor(stmt);

	/*  second query  */
	query = (SQLCHAR*)"SELECT USER_NAME();";
	ExecuteQuery(stmt, query);
	mappedUser = GetSingleResult(stmt, FALSE);
	internal_printf("[*] Mapped to the user %s\n", mappedUser);

	/*  Close the cursor  */
	ODBC32$SQLCloseCursor(stmt);

	/*  third query  */
	internal_printf("[*] Gathering roles...\n");
	query = (SQLCHAR*)"SELECT [name] from sysusers where issqlrole = 1;";
	ExecuteQuery(stmt, query);
	dbRoles = GetMultipleResults(stmt, FALSE);
	
	/*  Close the cursor  */
	ODBC32$SQLCloseCursor(stmt);
	
	/*  fifth query (loop)  */
	for (int i = 0; dbRoles[i] != NULL; i++) {
		char* role = dbRoles[i];
		char query[1024];
		MSVCRT$sprintf(query, "SELECT IS_MEMBER('%s');", role);
		ExecuteQuery(stmt, query);
		PrintMemberStatus(role, GetSingleResult(stmt, FALSE));
		ODBC32$SQLCloseCursor(stmt);
	}
	

	/*  fifth query (loop)  */
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
	if(!bofstart())
	{
		return;
	}
	
	Whoami();

	printoutput(TRUE);
};

#else

int main()
{
	Whoami();
}

#endif
