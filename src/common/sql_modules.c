//
// imported by bof entry files to check or toggle status of modules
// include statmenet should be after the sql.c include
//
#include <windows.h>
#include <sql.h>
#include "bofdefs.h"

//
// Configure sp_serveroption or sp_configure
//
BOOL ToggleModule(SQLHSTMT stmt, char* name, char* value, char* link, char* impersonate) {
    if (MSVCRT$strcmp(name, "rpc") == 0)
    {
        char* prefix = "EXEC sp_serveroption '";
        char* middle = "', 'rpc out', '";
        char* suffix = "';";

        char* query = (char*)MSVCRT$malloc((MSVCRT$strlen(prefix) + MSVCRT$strlen(link) + MSVCRT$strlen(middle) + MSVCRT$strlen(value) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
        MSVCRT$strcpy(query, prefix);
        MSVCRT$strcat(query, link);
        MSVCRT$strcat(query, middle);
        MSVCRT$strcat(query, value);
        MSVCRT$strcat(query, suffix);

        //
        // link will always be passed as NULL here
        //
        return HandleQuery(stmt, (SQLCHAR*)query, NULL, impersonate, FALSE);
    }
    else
    {
        char* reconfigure = "RECONFIGURE;";
        char* advOpts = "EXEC sp_configure 'show advanced options', 1;";
        
        //
        // setting TRUE for using rpc query
        //
        if (!HandleQuery(stmt, (SQLCHAR*)advOpts, link, impersonate, TRUE))
        {
            return FALSE;
        }


        //
        // Close the cursor
        //
        ClearCursor(stmt);
        ODBC32$SQLCloseCursor(stmt);

        //
        // Reconfigure
        //
        if (!HandleQuery(stmt, (SQLCHAR*)reconfigure, link, impersonate, TRUE))
        {
            return FALSE;
        }

        //
        // Close the cursor
        //
        ClearCursor(stmt);
        ODBC32$SQLCloseCursor(stmt);

        //
        // toggle on or off
        //
        char* prefix = "EXEC sp_configure '";
        char* middle = "', ";
        char* suffix = ";";
        
        char* query = (char*)MSVCRT$malloc((MSVCRT$strlen(prefix) + MSVCRT$strlen(name) + MSVCRT$strlen(middle) + MSVCRT$strlen(value) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
        MSVCRT$strcpy(query, prefix);
        MSVCRT$strcat(query, name);
        MSVCRT$strcat(query, middle);
        MSVCRT$strcat(query, value);
        MSVCRT$strcat(query, suffix);

        if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, TRUE))
        {
            return FALSE;
        }

        //
        // Close the cursor
        //
        ClearCursor(stmt);
        ODBC32$SQLCloseCursor(stmt);

        //
        // Reconfigure
        //
        return HandleQuery(stmt, (SQLCHAR*)reconfigure, link, impersonate, TRUE);
    }
}

//
// Query the status of RPC on a linked server
//
BOOL CheckRpcOnLink(SQLHSTMT stmt, char* link, char* impersonate)
{
    char* prefix = "SELECT is_rpc_out_enabled FROM sys.servers WHERE lower(name) like '%";
    char* suffix = "%';";

    char* query = (char*)MSVCRT$malloc((MSVCRT$strlen(prefix) + MSVCRT$strlen(link) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
    MSVCRT$strcpy(query, prefix);
    MSVCRT$strcat(query, link);
    MSVCRT$strcat(query, suffix);

    return HandleQuery(stmt, (SQLCHAR*)query, NULL, impersonate, FALSE);

}

//
// Execute query and hold results for printing
//
BOOL CheckModuleStatus(SQLHSTMT stmt, char* name, char* link, char* impersonate)
{
    char* prefix = "SELECT CAST(name AS NVARCHAR(128)) AS name, "
                   "CAST(value_in_use AS INT) AS value_in_use\n"
                   "FROM sys.configurations\n"
                   "WHERE name = '";
    char* suffix = "';";

    char* query = (char*)MSVCRT$malloc((MSVCRT$strlen(prefix) + MSVCRT$strlen(name) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
    MSVCRT$strcpy(query, prefix);
    MSVCRT$strcat(query, name);
    MSVCRT$strcat(query, suffix);

    return HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, FALSE);
}

//
// Execute query and return an int
// Used to verify module status before executing
// 0 = disabled 1 = enabled -1 = error
//
int GetModuleStatus(SQLHSTMT stmt, char* name, char* link, char* impersonate)
{
    char* prefix = "SELECT CAST(value_in_use AS INT) AS value_in_use "
                  "FROM sys.configurations "
                  "WHERE name = '";
    char* suffix = "';";

    char* query = (char*)MSVCRT$malloc((MSVCRT$strlen(prefix) + MSVCRT$strlen(name) + MSVCRT$strlen(suffix) + 1) * sizeof(char));
    MSVCRT$strcpy(query, prefix);
    MSVCRT$strcat(query, name);
    MSVCRT$strcat(query, suffix);

    if (!HandleQuery(stmt, (SQLCHAR*)query, link, impersonate, FALSE))
    {
        return -1;
    }

    return MSVCRT$atoi(GetSingleResult(stmt, FALSE));
}