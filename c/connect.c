// gcc -o connect connect.c -I../instantclient/sdk/include -L../instantclient -lclntsh -Wl,-rpath=../instantclient
// export LD_LIBRARY_PATH=/home/mint/Desktop/oci/OCI-Connection/instantclient:$LD_LIBRARY_PATH 
// instant client is in ../instantclient
// wallet is in ../wallet

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oci.h>

void print_oci_error(OCIError *errhp);
int read_key_file(char *username, char *password, char *wallet_pw);
void clean_up(OCISession *usrhp, OCISvcCtx *svchp, OCIServer *srvhp, OCIError *errhp, OCIEnv *envhp);
int set_environment();
void get_column_names(OCIEnv* envhp, OCISvcCtx* svchp, OCIError* errhp, const char* table_name);

int main() {
    // Initialize OCI environment
    OCIEnv *envhp;
    OCIServer *srvhp;
    OCISession *usrhp;
    OCISvcCtx *svchp;
    OCIStmt *stmthp;
    OCIDefine *defnp;
    OCIError *errhp;

    if(set_environment() != 0) {
        printf("Error setting environment.\n");
        return 1;
    }

    sword status;
    status = OCIEnvCreate(&envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
    if(status != OCI_SUCCESS) {
        printf("OCIEnvCreate failed.\n");
        return 1;
    }

    // Read keys from key file
    char username[128], password[128], wallet_pw[128];
    if(read_key_file(username, password, wallet_pw) == 0) {
        printf("Read key file successfully.\n");
        printf("Username: %s\n", username);
        // printf("Password: %s\n", password);
        // printf("Wallet Password: %s\n", wallet_pw);
    } else {
        printf("Error reading key file.\n");
    }

    // Allocate handles
    OCIHandleAlloc(envhp, (void **)&errhp, OCI_HTYPE_ERROR, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&srvhp, OCI_HTYPE_SERVER, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&svchp, OCI_HTYPE_SVCCTX, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&usrhp, OCI_HTYPE_SESSION, 0, NULL);

    // Attach to server
    // printf("Attaching server...\n");
    status = OCIServerAttach(srvhp, errhp, (text *)"cbdcauto_low", strlen("cbdcauto_low"), OCI_DEFAULT);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // Set attribute server context
    // printf("Setting attribute server context...\n");
    status = OCIAttrSet(svchp, OCI_HTYPE_SVCCTX, srvhp, 0, OCI_ATTR_SERVER, errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // Set attribute session context
    // printf("Setting attribute session context...\n");
    status = OCIAttrSet(usrhp, OCI_HTYPE_SESSION, username, strlen(username), OCI_ATTR_USERNAME, errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }
    status = OCIAttrSet(usrhp, OCI_HTYPE_SESSION, password, strlen(password), OCI_ATTR_PASSWORD, errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // Log in
    // printf("Logging in...\n");
    status = OCISessionBegin(svchp, errhp, usrhp, OCI_CRED_RDBMS, OCI_DEFAULT);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }
    status = OCIAttrSet(svchp, OCI_HTYPE_SVCCTX, usrhp, 0, OCI_ATTR_SESSION, errhp);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    printf("Connected to Oracle Database.\n");
    printf("Select an option: \n");
    printf("1. Enter SQL statement.\n");
    printf("2. Enter SQL commands file.\n");
    printf("3. Exit.\n");

    int option;
    char sql_statement[256];
    char sql_commands_file[256];

    while(scanf("%d", &option) != 1 || option < 1 || option > 3) {
        printf("Invalid input. Please enter 1, 2, or 3.\n");
        while(getchar() != '\n');
    }

    // if option is 1, take user input for sql statement
    if(option == 1) {
        printf("Enter SQL statement (dont add ';'): ");
        // get user input
        scanf(" %[^\n]s", sql_statement);
        sql_statement[sizeof(sql_statement) - 1] = '\0';  // Add null terminator
        printf("SQL statement: %s\n", sql_statement);
    }
    if(option == 2) {
        printf("Enter SQL commands file (unimplemented): ");
        // char sql_commands_file[256];
        scanf("%s", sql_commands_file);
        printf("SQL commands file: %s\n", sql_commands_file);
    }
    if(option == 3) {
        clean_up(usrhp, svchp, srvhp, errhp, envhp);
        printf("Exiting...\n");
        return 0;
    }




    // Execute SELECT statement
    // printf("Preparing SELECT statement...\n");
    status = OCIHandleAlloc(envhp, (void **)&stmthp, OCI_HTYPE_STMT, 0, NULL);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }
    status = OCIStmtPrepare(stmthp, errhp, (text *)sql_statement, strlen(sql_statement), OCI_NTV_SYNTAX, OCI_DEFAULT);
    if(status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // printf("Executing statement...\n");
    // Define output variables
    ub4 id;
    text name[20];
    OCIDefineByPos(stmthp, &defnp, errhp, 1, &id, sizeof(id), SQLT_INT, 0, 0, 0, OCI_DEFAULT);
    OCIDefineByPos(stmthp, &defnp, errhp, 2, &name, sizeof(name), SQLT_STR, 0, 0, 0, OCI_DEFAULT);
    
    // Execute the statement and fetch rows
    status = OCIStmtExecute(svchp, stmthp, errhp, 0, 0, NULL, NULL, OCI_DEFAULT);
    if(status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
        printf("Error executing SQL statement.\n");
        print_oci_error(errhp);
        clean_up(usrhp, svchp, srvhp, errhp, envhp);
        return 1;
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
            return 1;
        }

        printf("%d\t%s\n", id, name);
    }

    OCIHandleFree(stmthp, OCI_HTYPE_STMT);
    OCISessionEnd(svchp, errhp, usrhp, OCI_DEFAULT);
    OCIServerDetach(srvhp, errhp, OCI_DEFAULT);
    clean_up(usrhp, svchp, srvhp, errhp, envhp);
    printf("Disconnected from Oracle Database.\n");
    return 0;
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
void clean_up(OCISession *usrhp, OCISvcCtx *svchp, OCIServer *srvhp, OCIError *errhp, OCIEnv *envhp) {
    OCIHandleFree(usrhp, OCI_HTYPE_SESSION);
    OCIHandleFree(svchp, OCI_HTYPE_SVCCTX);
    OCIHandleFree(srvhp, OCI_HTYPE_SERVER);
    OCIHandleFree(errhp, OCI_HTYPE_ERROR);
    OCIHandleFree(envhp, OCI_HTYPE_ENV);
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


// SELECT column_name FROM user_tab_columns WHERE table_name = test
// void get_column_names(OCIEnv* envhp, OCISvcCtx* svchp, OCIError* errhp, const char* table_name) {
//     OCIStmt* stmthp;
//     sword status;

//     // Prepare the SQL statement to retrieve column names
//     const char* sql_statement = "SELECT column_name FROM user_tab_columns WHERE table_name = :table_name";
//     status = OCIHandleAlloc((dvoid *)envhp, (dvoid **)&stmthp, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0);
//     if (status != OCI_SUCCESS) {
//         printf("OCIHandleAlloc failed.\n");
//         return;
//     }
//     status = OCIStmtPrepare(stmthp, errhp, (text *)sql_statement, (ub4)strlen(sql_statement), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT);
//     if (status != OCI_SUCCESS) {
//         printf("OCIStmtPrepare failed.\n");
//         return;
//     }

//     // Bind the table name to the statement
//     OCIBind* bindp;
//     const int table_name_length = strlen(table_name);
//     status = OCIBindByName(stmthp, &bindp, errhp, (text *)":table_name", -1, (void *)table_name, table_name_length, SQLT_STR, 0, 0, 0, 0, 0, OCI_DEFAULT);
//     if (status != OCI_SUCCESS) {
//         printf("OCIBindByName failed.\n");
//         return;
//     }

//     // Execute the statement
//     status = OCIStmtExecute(svchp, stmthp, errhp, (ub4)1, (ub4)0, (OCISnapshot *)NULL, (OCISnapshot *)NULL, (ub4)OCI_DEFAULT);
//     if (status != OCI_SUCCESS) {
//         printf("OCIStmtExecute failed.\n");
//         print_oci_error(errhp);
//         return;
//     }

//     // Fetch and print column names
//     OCIParam* paramhp;
//     ub2 num_cols;
//     ub2 i;
//     status = OCIStmtFetch(stmthp, errhp, (ub4)1, (ub4)OCI_FETCH_NEXT, (ub4)OCI_DEFAULT);
//     if (status != OCI_SUCCESS) {
//         printf("OCIStmtFetch failed.\n");
//         return;
//     }
//     status = OCIAttrGet((dvoid *)stmthp, (ub4)OCI_HTYPE_STMT, (dvoid *)&paramhp, (ub4 *)0, (ub4)OCI_ATTR_PARAM, errhp);
//     if (status != OCI_SUCCESS) {
//         printf("OCIAttrGet failed.\n");
//         return;
//     }
//     status = OCIAttrGet((dvoid *)paramhp, (ub4)OCI_DTYPE_PARAM, (dvoid *)&num_cols, (ub4 *)0, (ub4)OCI_ATTR_NUM_PARAMS, errhp);
//     if (status != OCI_SUCCESS) {
//         printf("OCIAttrGet failed.\n");
//         return;
//     }

//     printf("Column Names:\n");
//     for (i = 1; i <= num_cols; i++) {
//         text* col_name;
//         ub4 col_name_len;
//         status = OCIParamGet((dvoid *)paramhp, (ub4)OCI_DTYPE_PARAM, errhp, (dvoid **)&paramhp, (ub4)i);
//         if (status != OCI_SUCCESS) {
//             printf("OCIParamGet failed.\n");
//             return;
//         }
//         status = OCIAttrGet((dvoid *)paramhp, (ub4)OCI_DTYPE_PARAM, (dvoid *)&col_name, (ub4 *)&col_name_len, (ub4)OCI_ATTR_NAME, errhp);
//         if (status != OCI_SUCCESS) {
//             printf("OCIAttrGet failed.\n");
//             return;
//         }
//         printf("%.*s\n", col_name_len, col_name);
//     }
// }


