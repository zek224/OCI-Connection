#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "database_connection.h"

typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
} string_buffer;

void string_buffer_init(string_buffer *sb) {
    sb->length = 0;
    sb->capacity = 128;
    sb->buffer = malloc(sb->capacity);
    sb->buffer[0] = '\0';
}

void string_buffer_append(string_buffer *sb, const char *str) {
    size_t len = strlen(str);
    while (sb->length + len + 1 > sb->capacity) {
        sb->capacity *= 2;
        sb->buffer = realloc(sb->buffer, sb->capacity);
    }
    memcpy(sb->buffer + sb->length, str, len);
    sb->length += len;
    sb->buffer[sb->length] = '\0';
}

void string_buffer_free(string_buffer *sb) {
    free(sb->buffer);
}


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

char* execute_sql_query(sword status, OCIEnv *envhp, OCISvcCtx *svchp, OCIError *errhp, OCIServer *srvhp, OCISession *usrhp, const char *sql_query) {
    OCIStmt *stmthp;
    OCIDefine *defnp;

    // String buffer
    string_buffer result;
    string_buffer_init(&result);


    // Check if the query is a SELECT statement
    int is_select = (strncasecmp(sql_query, "SELECT", 6) == 0);

    // ===============================
    // PREPARE STATEMENT SECTION
    // Allocate a statement handle
    status = OCIHandleAlloc(envhp, (void **)&stmthp, OCI_HTYPE_STMT, 0, NULL);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return NULL;
    }
    status = OCIStmtPrepare(stmthp, errhp, (text *)sql_query, strlen(sql_query), OCI_NTV_SYNTAX, OCI_DEFAULT);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return NULL;
    }
    // End of Prepare Statement 
    // ===============================


    ub4 column_count = 0;
    char** column_values = NULL;
    ub2* column_lengths = NULL;

    if(is_select) {
        // ===============================
        // EXECUTE SELECT STATEMENT SECTION
        // Execute the statement
        status = OCIStmtExecute(svchp, stmthp, errhp, 0, 0, NULL, NULL, OCI_DEFAULT);
        if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
            printf("Error executing SQL statement.\n --> OCI_SUCCESS: %d\n --> OCI_SUCCESS_WITH_INFO: %d\n", OCI_SUCCESS, OCI_SUCCESS_WITH_INFO);
            print_oci_error(errhp);
            return NULL;
        }

        // ===============================
        // COLUMN NAMES SECTION
        // Get column count
        status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &column_count, 0, OCI_ATTR_PARAM_COUNT, errhp);
        if (status != OCI_SUCCESS) {
            printf("Error getting column count.\n");
            print_oci_error(errhp);
            return NULL;
        }

        // Define output variables
        OCIDefine *defines[column_count];
        ub2 data_types[column_count];
        ub2 data_sizes[column_count];
        ub2 data_lengths[column_count];
        char column_names[column_count][30];
        column_values = malloc(column_count * sizeof(char*));
        column_lengths = malloc(column_count * sizeof(ub2));

        printf("Query executed successfully.\n");

        for (ub4 i = 0; i < column_count; ++i) {
            status = OCIParamGet(stmthp, OCI_HTYPE_STMT, errhp, (dvoid **)&defines[i], i + 1);
            if (status != OCI_SUCCESS) {
                printf("Error getting column parameter.\n");
                print_oci_error(errhp);
                return NULL;
            }

            status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, &data_types[i], 0, OCI_ATTR_DATA_TYPE, errhp);
            if (status != OCI_SUCCESS) {
                printf("Error getting column data type.\n");
                print_oci_error(errhp);
                return NULL;
            }

            status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, &data_sizes[i], 0, OCI_ATTR_DATA_SIZE, errhp);
            if (status != OCI_SUCCESS) {
                printf("Error getting column data size.\n");
                print_oci_error(errhp);
                return NULL;
            }

            ub4 name_length;
            status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, &name_length, 0, OCI_ATTR_NAME, errhp);
            if (status != OCI_SUCCESS) {
                printf("Error getting column name length.\n");
                print_oci_error(errhp);
                return NULL;
            }

            status = OCIAttrGet(defines[i], OCI_DTYPE_PARAM, column_names[i], &name_length, OCI_ATTR_NAME, errhp);
            if (status != OCI_SUCCESS) {
                printf("Error getting column name.\n");
                print_oci_error(errhp);
                return NULL;
            }
        }

        // Print column names
        OCIParam *paramhp;
        ub4 num_cols;
        status = OCIAttrGet(stmthp, OCI_HTYPE_STMT, &num_cols, 0, OCI_ATTR_PARAM_COUNT, errhp);
        if (status != OCI_SUCCESS) {
            printf("Error getting column count.\n");
            print_oci_error(errhp);
            return NULL;
        }

        // Print column names
        int column_width = 15;
        printf("%-*s", column_width, "Returned Data:\n");
        for (ub4 i = 1; i <= num_cols; ++i) {
            status = OCIParamGet(stmthp, OCI_HTYPE_STMT, errhp, (void **)&paramhp, i);
            if (status != OCI_SUCCESS) {
                printf("Error getting column parameter.\n");
                print_oci_error(errhp);
                return NULL;
            }

            text *column_name;
            ub4 column_name_length;
            status = OCIAttrGet(paramhp, OCI_DTYPE_PARAM, &column_name, &column_name_length, OCI_ATTR_NAME, errhp);
            if (status != OCI_SUCCESS) {
                printf("Error getting column name.\n");
                print_oci_error(errhp);
                return NULL;
            }

            printf("%-*s", column_width, column_name);

            // fill string buffer
            char column_buf[column_width + 1];
            snprintf(column_buf, sizeof(column_buf), "%-*s", column_width, column_name);
            string_buffer_append(&result, column_buf);
        }
        string_buffer_append(&result, "\n");
        // END OF COLUMN NAMES SECTION
        // ===============================


        // ===============================
        // COLUMN VALUES SECTION
        // Allocate memory for column values and lengths

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
                // fill string buffer
                char value_buf[column_width + 1];
                snprintf(value_buf, sizeof(value_buf), "%-*s", column_width, column_values[i]);
                string_buffer_append(&result, value_buf);
                // string_buffer_append(&result, "\n");
            }
            printf("\n");
            string_buffer_append(&result, "\n");
        }
        // END OF COLUMN VALUES SECTION
        // ===============================

    // END OF SELECT STATEMENT SECTION
    // ===============================


    } else {
        // ===============================
        // EXECUTE NON SELECT STATEMENT SECTION
        // Execute the statement
        status = OCIStmtExecute(svchp, stmthp, errhp, 1, 0, NULL, NULL, OCI_DEFAULT);
        if (status != OCI_SUCCESS) {
            printf("(not a select statement) Error executing SQL statement.\n --> OCI_SUCCESS: %d\n --> OCI_SUCCESS_WITH_INFO: %d\n", OCI_SUCCESS, OCI_SUCCESS_WITH_INFO);
            print_oci_error(errhp);
            return NULL;
        }
        printf("SQL statement executed successfully.\n");

        // Commit the transaction
        status = OCITransCommit(svchp, errhp, OCI_DEFAULT);
        if (status != OCI_SUCCESS) {
            printf("Error committing transaction.\n");
            print_oci_error(errhp);
            return NULL;
        }
        printf("Transaction committed successfully.\n");
        // END OF NON SELECT STATEMENT SECTION
        // ===============================
    }

    // string_buffer_append(&result, '\0');
    char *result_copy = (char *)malloc((result.length + 1) * sizeof(char));
    memcpy(result_copy, result.buffer, result.length);
    result_copy[result.length] = '\0';
    free(result.buffer);
    return result_copy;

    // return result.buffer;

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
