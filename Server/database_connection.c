// gcc -o database_connection database_connection.c -I../instantclient/sdk/include -L../instantclient -lclntsh -Wl,-rpath=../instantclient
// export LD_LIBRARY_PATH="/mnt/c/Users/Zeeshan Khan/Desktop/p/oracle/OCI-Connection/instantclient":$LD_LIBRARY_PATH 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database_connection.h"


int initialize(OCIEnv **envhp, OCIServer **srvhp, OCISession **usrhp, OCISvcCtx **svchp, OCIError **errhp, sword *status) {
    // const char *sql_statement = argv[1];

    if(set_environment() != 0) {
        printf("Error setting environment.\n");
        return 1;
    }

    *status = OCIEnvCreate(envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
    if(*status != OCI_SUCCESS) {
        printf("OCIEnvCreate failed.\n");
        return 1;
    }

    // Read keys from key file
    char username[128], password[128], wallet_pw[128];
    if(read_key_file(username, password, wallet_pw) == 0) {
        printf("Read key file successfully.\n");
        printf("Username: %s\n", username);
    } else {
        printf("Error reading key file.\n");
        return 1;
    }
    // Connect to the database
    connect_to_db(*status, *envhp, srvhp, usrhp, svchp, errhp, username, password);
    return 0;
}

void execute_sql_query(sword status, OCIEnv *envhp, OCISvcCtx *svchp, OCIError *errhp, OCIServer *srvhp, OCISession *usrhp, const char *sql_query) {
    OCIStmt *stmthp;
    OCIDefine *defnp;

    // Execute SELECT statement
    status = OCIHandleAlloc(envhp, (void **)&stmthp, OCI_HTYPE_STMT, 0, NULL);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return;
    }
    status = OCIStmtPrepare(stmthp, errhp, (text *)sql_query, strlen(sql_query), OCI_NTV_SYNTAX, OCI_DEFAULT);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return;
    }

    // Define output variables
    ub4 id;
    text name[20];
    OCIDefineByPos(stmthp, &defnp, errhp, 1, &id, sizeof(id), SQLT_INT, 0, 0, 0, OCI_DEFAULT);
    OCIDefineByPos(stmthp, &defnp, errhp, 2, &name, sizeof(name), SQLT_STR, 0, 0, 0, OCI_DEFAULT);

    // Execute the statement and fetch rows
    status = OCIStmtExecute(svchp, stmthp, errhp, 0, 0, NULL, NULL, OCI_DEFAULT);
    if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
        printf("Error executing SQL statement.\n");
        print_oci_error(errhp);
        clean_up(usrhp, svchp, srvhp, errhp, envhp);
        return;
    }

    // Print results
    printf("ID\tName\n");
    printf("--------\n");
    while(1) {
        status = OCIStmtFetch(stmthp, errhp, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
        if(status == OCI_NO_DATA) {
            break;
        } else if(status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
            print_oci_error(errhp);
            return;
        }

        printf("%d\t%s\n", id, name);
    }

    OCIHandleFree(stmthp, OCI_HTYPE_STMT);
}



void connect_to_db(sword status, OCIEnv *envhp, OCIServer **srvhp, OCISession **usrhp, OCISvcCtx **svchp, OCIError **errhp, const char *username, const char *password) {
    // Allocate handles
    OCIHandleAlloc(envhp, (void **)errhp, OCI_HTYPE_ERROR, 0, NULL);
    OCIHandleAlloc(envhp, (void **)srvhp, OCI_HTYPE_SERVER, 0, NULL);
    OCIHandleAlloc(envhp, (void **)svchp, OCI_HTYPE_SVCCTX, 0, NULL);
    OCIHandleAlloc(envhp, (void **)usrhp, OCI_HTYPE_SESSION, 0, NULL);

    // Attach to server
    status = OCIServerAttach(*srvhp, *errhp, (text *)"cbdcauto_low", strlen("cbdcauto_low"), OCI_DEFAULT);
    if(status != OCI_SUCCESS) {
        print_oci_error(*errhp);
        return;
    }

    // Set attribute server context
    status = OCIAttrSet(*svchp, OCI_HTYPE_SVCCTX, *srvhp, 0, OCI_ATTR_SERVER, *errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(*errhp);
        return;
    }

    // Set attribute session context
    status = OCIAttrSet(*usrhp, OCI_HTYPE_SESSION, (void *)username, strlen(username), OCI_ATTR_USERNAME, *errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(*errhp);
        return;
    }
    status = OCIAttrSet(*usrhp, OCI_HTYPE_SESSION, (void *)password, strlen(password), OCI_ATTR_PASSWORD, *errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(*errhp);
        return;
    }

    // Log in
    status = OCISessionBegin(*svchp, *errhp, *usrhp, OCI_CRED_RDBMS, OCI_DEFAULT);
    if(status != OCI_SUCCESS) {
        print_oci_error(*errhp);
        return;
    }
    status = OCIAttrSet(*svchp, OCI_HTYPE_SVCCTX, *usrhp, 0, OCI_ATTR_SESSION, *errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(*errhp);
        return;
    }

    printf("Connected to Oracle Database.\n");
}

// Prints OCI error
void print_oci_error(OCIError *errhp) {
    sb4 errcode = 0;
    text errbuf[512];
    OCIErrorGet((dvoid *)errhp, 1, (text *)NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
    printf("Error %d: %s\n", errcode, errbuf);
}

// Reads Key File into username, password, and wallet_pw
int read_key_file(char *username, char *password, char *wallet_pw) {
    FILE *key_file = fopen("../key.txt", "r");
    if(!key_file) {
        printf("Error opening key file.\n");
        return 1;
    }
    char line[256];
    while(fgets(line, sizeof(line), key_file)) {
        if(strncmp(line, "username", 8) == 0) {
            sscanf(line, "username%*[ ]=%*[ ]%s", username);
        } else if(strncmp(line, "password", 8) == 0) {
            sscanf(line, "password%*[ ]=%*[ ]%s", password);
        } else if(strncmp(line, "wallet_password", 15) == 0) {
            sscanf(line, "wallet_password%*[ ]=%*[ ]%s", wallet_pw);
        }
    }
    fclose(key_file);
    return 0;
}

// Cleans up OCI handles
void clean_up(OCISession **usrhp, OCISvcCtx **svchp, OCIServer **srvhp, OCIError **errhp, OCIEnv **envhp) {
    OCIHandleFree(*usrhp, OCI_HTYPE_SESSION);
    OCIHandleFree(*svchp, OCI_HTYPE_SVCCTX);
    OCIHandleFree(*srvhp, OCI_HTYPE_SERVER);
    OCIHandleFree(*errhp, OCI_HTYPE_ERROR);
    OCIHandleFree(*envhp, OCI_HTYPE_ENV);
}

int set_environment() {
    // Set TNS_ADMIN environment variable
    if(setenv("TNS_ADMIN", "../wallet", 1) != 0) {
        perror("Error setting TNS_ADMIN environment variable");
        return 1;
    }

    // Set LD_LIBRARY_PATH environment variable
    if(setenv("LD_LIBRARY_PATH", "../instantclient/", 1) != 0) {
        perror("Error setting LD_LIBRARY_PATH environment variable");
        return 1;
    }
    return 0;
}

void disconnect_from_db(OCISession *usrhp, OCISvcCtx *svchp, OCIServer *srvhp, OCIError *errhp, OCIEnv *envhp) {
    OCISessionEnd(svchp, errhp, usrhp, OCI_DEFAULT);
    OCIServerDetach(srvhp, errhp, OCI_DEFAULT);
    // clean_up(usrhp, svchp, srvhp, errhp, envhp);
    printf("Disconnected from Oracle Database.\n");
}











// int main(int argc, char *argv[]) {
//     if (argc < 2) {
//         printf("Usage: %s <SQL statement>\n", argv[0]);
//         return 1;
//     }

//     const char *sql_statement = argv[1];

//     // Initialize OCI environment
//     OCIEnv *envhp;
//     OCIServer *srvhp;
//     OCISession *usrhp;
//     OCISvcCtx *svchp;
//     OCIError *errhp;

//     if(set_environment() != 0) {
//         printf("Error setting environment.\n");
//         return 1;
//     }

//     sword status;
//     status = OCIEnvCreate(&envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
//     if(status != OCI_SUCCESS) {
//         printf("OCIEnvCreate failed.\n");
//         return 1;
//     }

//     // Read keys from key file
//     char username[128], password[128], wallet_pw[128];
//     if(read_key_file(username, password, wallet_pw) == 0) {
//         printf("Read key file successfully.\n");
//         printf("Username: %s\n", username);
//     } else {
//         printf("Error reading key file.\n");
//     }

//     // Connect to the database
//     connect_to_db(envhp, &srvhp, &usrhp, &svchp, &errhp, username, password);

//     // Execute the SQL query
//     // enter an sql statement 
//     execute_sql_query(status, envhp, svchp, errhp, srvhp, usrhp, sql_statement);

//     disconnect_from_db(usrhp, svchp, srvhp, errhp, envhp);

//     // Clean up and disconnect
//     clean_up(usrhp, svchp, srvhp, errhp, envhp);

//     printf("Disconnected from Oracle Database.\n");
//     return 0;
// }