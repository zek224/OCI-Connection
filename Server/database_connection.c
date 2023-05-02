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

    // ===============================
    // PREPARE STATEMENT SECTION
    // Allocate a statement handle
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

    // Execute the statement
    status = OCIStmtExecute(svchp, stmthp, errhp, 0, 0, NULL, NULL, OCI_DEFAULT);
    if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
        printf("Error executing SQL statement.\n --> OCI_SUCCESS: %d\n --> OCI_SUCCESS_WITH_INFO: %d\n", OCI_SUCCESS, OCI_SUCCESS_WITH_INFO);
        print_oci_error(errhp);
        return;
    }
    // End of Prepare Statement and Executing SQL Statement
    // ===============================


    // ===============================
    // COLUMN NAMES SECTION
    // Get column count
    ub4 column_count;
    status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &column_count, 0, OCI_ATTR_PARAM_COUNT, errhp);
    if (status != OCI_SUCCESS) {
        printf("Error getting column count.\n");
        print_oci_error(errhp);
        return;
    }

    // Define output variables
    OCIDefine *defines[column_count];
    ub2 data_types[column_count];
    ub2 data_sizes[column_count];
    ub2 data_lengths[column_count];
    char column_names[column_count][30];

    for (ub4 i = 0; i < column_count; ++i) {
        status = OCIParamGet(stmthp, OCI_HTYPE_STMT, errhp, (dvoid **)&defines[i], i + 1);
        if (status != OCI_SUCCESS) {
            printf("Error getting column parameter.\n");
            print_oci_error(errhp);
            return;
        }

        status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, &data_types[i], 0, OCI_ATTR_DATA_TYPE, errhp);
        if (status != OCI_SUCCESS) {
            printf("Error getting column data type.\n");
            print_oci_error(errhp);
            return;
        }

        status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, &data_sizes[i], 0, OCI_ATTR_DATA_SIZE, errhp);
        if (status != OCI_SUCCESS) {
            printf("Error getting column data size.\n");
            print_oci_error(errhp);
            return;
        }

        ub4 name_length;
        status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, &name_length, 0, OCI_ATTR_NAME, errhp);
        if (status != OCI_SUCCESS) {
            printf("Error getting column name length.\n");
            print_oci_error(errhp);
            return;
        }

        status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, column_names[i], &name_length, OCI_ATTR_NAME, errhp);
        if (status != OCI_SUCCESS) {
            printf("Error getting column name.\n");
            print_oci_error(errhp);
            return;
        }
    }

    // Print column names
    // Below gets Column Names and prints them
    OCIParam *paramhp;
    ub4 num_cols;
    status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &num_cols, 0, OCI_ATTR_PARAM_COUNT, errhp);
    if (status != OCI_SUCCESS) {
        printf("Error getting column count.\n");
        print_oci_error(errhp);
        return;
    }

    // Print column names
    int column_width = 15;
    printf("%-*s", column_width, "Returned Data:\n");
    for (ub4 i = 1; i <= num_cols; ++i) {
        status = OCIParamGet(stmthp, OCI_HTYPE_STMT, errhp, (void **)&paramhp, i);
        if (status != OCI_SUCCESS) {
            printf("Error getting column parameter.\n");
            print_oci_error(errhp);
            return;
        }

        text *column_name;
        ub4 column_name_length;
        status = OCIAttrGet(paramhp, OCI_DTYPE_PARAM, &column_name, &column_name_length, OCI_ATTR_NAME, errhp);
        if (status != OCI_SUCCESS) {
            printf("Error getting column name.\n");
            print_oci_error(errhp);
            return;
        }

        printf("%-*s", column_width, column_name);
    }
    // END OF COLUMN NAMES SECTION
    // ===============================


    // ===============================
    // COLUMN VALUES SECTION
    // Allocate memory for column values and lengths
    char** column_values = malloc(column_count * sizeof(char*));
    ub2* column_lengths = malloc(column_count * sizeof(ub2));
    printf("\n");
    // Define output variables and allocate memory for each column value
    for (ub4 i = 0; i < column_count; ++i) {
        column_values[i] = malloc((data_sizes[i] + 1) * sizeof(char)); // +1 for null terminator
        memset(column_values[i], 0, (data_sizes[i] + 1) * sizeof(char)); // Ensure the string is null-terminated

        status = OCIDefineByPos(stmthp, &defines[i], errhp, i + 1, column_values[i], data_sizes[i] + 1, SQLT_STR, &column_lengths[i], 0, 0, OCI_DEFAULT);
        if (status != OCI_SUCCESS) {
            printf("Error defining column variable for column %d.\n", i + 1);
            print_oci_error(errhp);
            goto cleanup;
        }
    }

    // Fetch and print rows
    while (1) {
        status = OCIStmtFetch2(stmthp, errhp, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
        if (status == OCI_NO_DATA) {
            break;
        } else if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
            printf("Error fetching data.\n");
            print_oci_error(errhp);
            goto cleanup;
        }

        for (ub4 i = 0; i < column_count; ++i) {
            printf("%-*s", column_width, column_values[i]);
        }
        printf("\n");
    }
    // END OF COLUMN VALUES SECTION
    // ===============================

// ========================
// Clean up code
cleanup:
    // Free dynamically allocated memory
    for (ub4 i = 0; i < column_count; ++i) {
        free(column_values[i]);
    }
    free(column_values);
    free(column_lengths);

    if (stmthp != NULL) {
        OCIHandleFree(stmthp, OCI_HTYPE_STMT);
    }
}
// end of clean up code
// ========================



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