#include <windows.h>
#include <sql.h>
#include "bofdefs.h"

void ShowError(unsigned int handletype, const SQLHANDLE* handle) {
    SQLCHAR sqlstate[1024];
    SQLCHAR message[1024];
    if (SQL_SUCCESS == ODBC32$SQLGetDiagRec(handletype, (SQLHANDLE)handle, 1, sqlstate, NULL, message, 1024, NULL))
        internal_printf("Message: %s \nSQL State: %s\n", message, sqlstate);
}

void ExecuteQuery(SQLHSTMT stmt, SQLCHAR* query) {
    SQLRETURN ret;

    ret = ODBC32$SQLExecDirect(stmt, query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        internal_printf("Error executing query.\n");
        ShowError(SQL_HANDLE_STMT, stmt);
    }
}

char** GetMultipleResults(SQLHSTMT stmt, BOOL hasHeader) {
    char** results = (char**)MSVCRT$malloc(1024 * sizeof(char*));
    SQLCHAR buf[1024];
    SQLLEN indicator;
    int i = 0;

    while (ODBC32$SQLFetch(stmt) == SQL_SUCCESS) {
        ODBC32$SQLGetData(stmt, 1, SQL_C_CHAR, buf, sizeof(buf), &indicator);
        if (hasHeader && i == 0) {
            i++;
            continue;
        }
        results[i] = (char*)MSVCRT$malloc((MSVCRT$strlen((char*)buf) + 1) * sizeof(char));
        MSVCRT$strcpy(results[i], (char*)buf);
        i++;
    }
    results[i] = NULL; // Null terminate the array
    return results;
}

char* GetSingleResult(SQLHSTMT stmt, BOOL hasHeader) {
    SQLCHAR* buf = (SQLCHAR*)MSVCRT$malloc(1024 * sizeof(SQLCHAR));
    SQLLEN indicator;
    ODBC32$SQLFetch(stmt);
    ODBC32$SQLGetData(stmt, 1, SQL_C_CHAR, buf, 1024, &indicator);
    if (hasHeader) {
        ODBC32$SQLFetch(stmt);
        ODBC32$SQLGetData(stmt, 1, SQL_C_CHAR, buf, 1024, &indicator);
    }
    return (char*)buf;
}

void PrintQueryResults(SQLHSTMT stmt, BOOL hasHeader) {
    SQLSMALLINT columns;
    SQLRETURN ret;
    
    // Get the number of columns in the result set
    ret = ODBC32$SQLNumResultCols(stmt, &columns);
    if (!SQL_SUCCEEDED(ret)) {
        internal_printf("Error retrieving column count.\n");
        return;
    }

    SQLCHAR buffer[1024];
    SQLSMALLINT columnNameLength, dataType, decimalDigits, nullable;
    SQLULEN columnSize;

    // Print column headers
    if (hasHeader) {
        for (SQLSMALLINT i = 1; i <= columns; i++) {
            ODBC32$SQLDescribeCol(stmt, i, buffer, sizeof(buffer), &columnNameLength, &dataType, &columnSize, &decimalDigits, &nullable);
            internal_printf("%s\t", buffer);
        }
    }

    if (hasHeader) {
        internal_printf("\n");
        for (SQLSMALLINT i = 1; i <= columns; i++) {
            internal_printf("--------");
        }
        internal_printf("\n");
    }
    
    // Iterate over each row
    while (SQL_SUCCEEDED(ret = ODBC32$SQLFetch(stmt))) {
        // Iterate over each column
        for (SQLSMALLINT i = 1; i <= columns; i++) {
            SQLLEN indicator;
            // Retrieve column data
            ret = ODBC32$SQLGetData(stmt, i, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);
            if (SQL_SUCCEEDED(ret)) {
                // Print column data
                if (indicator == SQL_NULL_DATA) MSVCRT$strcpy((char*)buffer, "NULL");
                internal_printf("%s\t", buffer);
            }
        }
        internal_printf("\n");
    }
}

SQLHDBC ConnectToSqlServer(SQLHENV* env, char* server, char* dbName) {
    SQLRETURN ret;
    SQLCHAR connstr[1024];
    SQLHDBC dbc = NULL;

    // Allocate an environment handle
    ret = ODBC32$SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, env);

    // Set environment attribute
    ret = ODBC32$SQLSetEnvAttr(*env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    // Allocate a connection handle
    ret = ODBC32$SQLAllocHandle(SQL_HANDLE_DBC, *env, &dbc);
    
    MSVCRT$sprintf((char*)connstr, "DRIVER={SQL Server};SERVER=%s;DATABASE=%s;Trusted_Connection=Yes;", server, dbName);

    /* Connect to the database */
    internal_printf("[*] Connecting to %s:1433\n", server);
    ret = ODBC32$SQLDriverConnect(dbc, NULL, connstr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (SQL_SUCCEEDED(ret)) {
        internal_printf("[+] Successfully connected to database\n");
    } else {
        internal_printf("[-] Error connecting to database.\n");
        ShowError(SQL_HANDLE_DBC, dbc);
        dbc = NULL; // Set dbc to NULL to indicate failure
    }

    return dbc;
}


void DisconnectSqlServer(SQLHENV env, SQLHDBC dbc, SQLHSTMT stmt) {
	internal_printf("\n[*] Disconnecting from server\n");

	if (stmt != NULL) {
		ODBC32$SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	}

	if (dbc != NULL) {
		ODBC32$SQLDisconnect(dbc);
		ODBC32$SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	}

	if (env != NULL) {
		ODBC32$SQLFreeHandle(SQL_HANDLE_ENV, env);
	}

}