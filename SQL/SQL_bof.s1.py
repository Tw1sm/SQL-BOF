from hashlib import sha512
from typing import Dict, List, Optional, Tuple

from outflank_stage1.task.base_bof_task import BaseBOFTask
from outflank_stage1.task.enums import BOFArgumentEncoding
from outflank_stage1.task.exceptions import TaskInvalidArgumentsException

class SQLBOFTask():

    def __init__(self, name: str, base_binary_name: str = None):
        super().__init__(name, base_binary_name=base_binary_name)

        self.parser.add_argument(
            "server",
        )

    def add_common_args(self):
        self.parser.add_argument(
            "database",
            default="",
            nargs="?"
        )

        self.parser.add_argument(
            "linkedserver",
            default="",
            nargs="?"
        )

        self.parser.add_argument(
            "impersonate",
            default="",
            nargs="?"
        )

    def encode_common_args(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return [
            (BOFArgumentEncoding.STR, parser_arguments.server),
            (BOFArgumentEncoding.STR, parser_arguments.database),
            (BOFArgumentEncoding.STR, parser_arguments.linkedserver),
            (BOFArgumentEncoding.STR, parser_arguments.impersonate),
        ]

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        return self.encode_common_args(arguments)

    def split_arguments(self, arguments: Optional[str], strip_quotes: bool = True) -> List[str]:
        # warning: this does only strip double quotes, single quotes are passed on
        return super().split_arguments(arguments, strip_quotes)

class SQL1434udpBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-1434udp", "./1434udp/1434udp")

        self.parser.description = "Enumerate SQL Server connection info"

        self.parser.epilog = "Must provide an IP; hostnames are not accepted"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return [
            (BOFArgumentEncoding.STR, parser_arguments.server),
        ]

class SQLAdsiBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-adsi", "./adsi/adsi")

        self.parser.add_argument(
            "adsiserver",
        )

        self.parser.add_argument(
            "port",
            default=4444,
            nargs="?"
        )

        self.add_common_args()

        self.parser.description = "Obtain ADSI creds from ADSI linked server"

        self.parser.epilog = "Port defaults to 4444. This gets opened on the SQL Server with the ADSI link, not the beaconing host"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.adsiserver),
            (BOFArgumentEncoding.STR, parser_arguments.port),
        ]

class SQLAgentCmdBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-agentcmd", "./agentcmd/agentcmd")

        self.parser.add_argument(
            "command",
        )

        self.add_common_args()

        self.parser.description = "Execute a system command using agent jobs"

        self.parser.epilog = "Output is not returned"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.command),
        ]

class SQLAgentStatusBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-agentstatus", "./agentstatus/agentstatus")

        self.add_common_args()

        self.parser.description = "Enumerate SQL agent status and jobs"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        return self.encode_common_args(arguments)

class SQLCheckRpcBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-checkrpc", "./checkrpc/checkrpc")

        self.add_common_args()

        self.parser.description = "Enumerate RPC status of linked servers"

class SQLClrBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-clr", "./clr/clr")

        # not needed, will always be asked to upload assembly through GUI
        #
        # self.parser.add_argument(
        #     "dllpath",
        # )

        self.parser.add_argument(
            "function",
            help="name of the stored procedure to create"
        )

        self.add_common_args()

        self.parser.description = "Load and execute .NET assembly in a stored procedure"

    def validate_files(self, arguments: List[str]):

        assembly = self.get_file_by_name("assembly")

        if assembly is None:
            raise TaskInvalidArgumentsException("No .NET assembly uploaded")

        if not assembly.original_name.endswith(".dll"):
            raise TaskInvalidArgumentsException("Uploaded file is not a DLL")

    def get_gui_elements(self) -> Optional[Dict]:
        return {
            "title": "Upload .NET assembly",
            "desc": "Load and execute .NET assembly in a stored procedure",
            "elements": [
                {
                    "name": "assembly",
                    "type": "file",
                    "description": ".NET assembly",
                    "placeholder": "Select .NET assembly",
                },
            ],
        }

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:

        assembly = self.get_file_by_name("assembly")

        self.append_response(f'[*] Running sql-clr BOF with .NET assembly: "{assembly.original_name}"\n\n')

        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.function),
            (BOFArgumentEncoding.STR, sha512(assembly.content).hexdigest()),
            (BOFArgumentEncoding.BUFFER, assembly.content),
        ]

class SQLColumnsBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-columns", "./columns/columns")

        self.parser.add_argument(
            "table",
        )

        self.add_common_args()

        self.parser.description = "Enumerate columns within a table"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        # cannot use helper function, because table is passed in middle of other arguments
        return [
            (BOFArgumentEncoding.STR, parser_arguments.server),
            (BOFArgumentEncoding.STR, parser_arguments.database),
            (BOFArgumentEncoding.STR, parser_arguments.table),
            (BOFArgumentEncoding.STR, parser_arguments.linkedserver),
            (BOFArgumentEncoding.STR, parser_arguments.impersonate),
        ]

class SQLDatabasesBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-databases", "./databases/databases")

        self.add_common_args()

        self.parser.description = "Enumerate databases on a server"

class SQLDisableClrBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-disableclr", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Disable CLR integration"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "clr enabled"),
            (BOFArgumentEncoding.STR, "0"),
        ]

class SQLEnableClrBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-enableclr", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Enable CLR integration"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "clr enabled"),
            (BOFArgumentEncoding.STR, "1"),
        ]

class SQLDisableOleBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-disableole", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Disable OLE Automation Procedures"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "Ole Automation Procedures"),
            (BOFArgumentEncoding.STR, "0"),
        ]

class SQLEnableOleBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-enableole", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Enable OLE Automation Procedures"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "Ole Automation Procedures"),
            (BOFArgumentEncoding.STR, "1"),
        ]

class SQLDisableRpcBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-disablerpc", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Disable RPC and RPC out on a linked server"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "rpc"),
            (BOFArgumentEncoding.STR, "FALSE"),
        ]

class SQLEnableRpcBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-enablerpc", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Enable RPC and RPC out on a linked server"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "rpc"),
            (BOFArgumentEncoding.STR, "TRUE"),
        ]

class SQLDisableXpBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-disablexp", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Disable xp_cmdshell"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "xp_cmdshell"),
            (BOFArgumentEncoding.STR, "0"),
        ]

class SQLEnableXpBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-enablexp", "./togglemodule/togglemodule")

        self.add_common_args()

        self.parser.description = "Enable xp_cmdshell"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, "xp_cmdshell"),
            (BOFArgumentEncoding.STR, "1"),
        ]

class SQLImperonsate(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-impersonate", "./impersonate/impersonate")

        self.parser.add_argument(
            "database",
            default="",
            nargs="?"
        )

        self.parser.description = "Enumerate users that can be impersonated"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return [
            (BOFArgumentEncoding.STR, parser_arguments.server),
            (BOFArgumentEncoding.STR, parser_arguments.database),
        ]

class SQLInfoBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-info", "./info/info")

        self.parser.add_argument(
            "database",
            default="",
            nargs="?"
        )

        self.parser.description = "Gather information about the SQL server"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return [
            (BOFArgumentEncoding.STR, parser_arguments.server),
            (BOFArgumentEncoding.STR, parser_arguments.database),
        ]

class SQLLinksBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-links", "./links/links")

        self.add_common_args()

        self.parser.description = "Enumerate linked servers"

class SQLOleCmdBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-olecmd", "./olecmd/olecmd")

        self.parser.add_argument(
            "command",
        )

        self.add_common_args()

        self.parser.description = "Execute a system command using OLE automation procedures"

        self.parser.epilog = "Output is not returned"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.command),
        ]

class SQLQueryBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-query", "./query/query")

        self.parser.add_argument(
            "query",
        )

        self.add_common_args()

        self.parser.description = "Execute a custom SQL query"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.query),
        ]

class SQLRowsBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-rows", "./rows/rows")

        self.parser.add_argument(
            "table",
        )

        self.add_common_args()

        self.parser.description = "Get the count of rows in a table"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        # cannot use helper function, because table is passed in middle of other arguments
        return [
            (BOFArgumentEncoding.STR, parser_arguments.server),
            (BOFArgumentEncoding.STR, parser_arguments.database),
            (BOFArgumentEncoding.STR, parser_arguments.table),
            (BOFArgumentEncoding.STR, parser_arguments.linkedserver),
            (BOFArgumentEncoding.STR, parser_arguments.impersonate),
        ]

class SQLSearchBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-search", "./search/search")

        self.parser.add_argument(
            "keyword",
        )

        self.add_common_args()

        self.parser.description = "Search a table for a column name"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.keyword),
        ]

class SQLSmbBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-smb", "./smb/smb")

        self.parser.add_argument(
            "listener",
        )

        self.add_common_args()

        self.parser.description = "Coerce NetNTLM auth via xp_dirtree"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.listener),
        ]

class SQLTablesBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-tables", "./tables/tables")

        self.add_common_args()

        self.parser.description = "Enumerate tables within a database"

class SQLUsersBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-users", "./users/users")

        self.add_common_args()

        self.parser.description = "Enumerate users with database access"

class SQLWhoamiBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-whoami", "./whoami/whoami")

        self.add_common_args()

        self.parser.description = "Gather information about the SQL server"

class SQLXpCmdBOF(SQLBOFTask, BaseBOFTask):

    def __init__(self):
        super().__init__("sql-xpcmd", "./xpcmd/xpcmd")

        self.parser.add_argument(
            "command",
        )

        self.add_common_args()

        self.parser.description = "Execute a system command using xp_cmdshell"

        self.parser.epilog = "Output returned unless using a linked server. WARNING: running a persistent command will cause the beacon to hang"

    def _encode_arguments_bof(self, arguments: List[str]) -> List[Tuple[BOFArgumentEncoding, str]]:
        parser_arguments = self.parser.parse_args(arguments)

        return self.encode_common_args(arguments) + [
            (BOFArgumentEncoding.STR, parser_arguments.command),
        ]
