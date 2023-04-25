#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oci.h>

void print_oci_error(OCIError *errhp);

int main() {
    // Set TNS_ADMIN environment variable
    if (setenv("TNS_ADMIN", "../wallet", 1) != 0) {
        perror("Error setting TNS_ADMIN environment variable");
        return 1;
    }

    // Set LD_LIBRARY_PATH environment variable
    if (setenv("LD_LIBRARY_PATH", "../instantclient/", 1) != 0) {
        perror("Error setting LD_LIBRARY_PATH environment variable");
        return 1;
    }

    // Initialize OCI environment
    OCIEnv *envhp;
    OCIServer *srvhp;
    OCISession *usrhp;
    OCISvcCtx *svchp;
    OCIStmt *stmthp;
    OCIDefine *defnp;
    OCIError *errhp;

    sword status;
    status = OCIEnvCreate(&envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
    if (status != OCI_SUCCESS) {
        printf("OCIEnvCreate failed.\n");
        return 1;
    }

    // Read keys from key file
    char username[128], password[128], wallet_pw[128];
    FILE *key_file = fopen("../key.txt", "r");
    if (!key_file) {
        printf("Error opening key file.\n");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), key_file)) {
        if (strncmp(line, "username", 8) == 0) {
            sscanf(line, "username%*[ ]=%*[ ]%s", username);
        } else if (strncmp(line, "password", 8) == 0) {
            sscanf(line, "password%*[ ]=%*[ ]%s", password);
        } else if (strncmp(line, "wallet_password", 15) == 0) {
            sscanf(line, "wallet_password%*[ ]=%*[ ]%s", wallet_pw);
        }
    }
    fclose(key_file);

    // print username, password, wallet_pw
    // printf("username: %s\n", username);
    // printf("password: %s\n", password);
    // printf("wallet_pw: %s\n", wallet_pw);

    // Allocate handles
    OCIHandleAlloc(envhp, (void **)&errhp, OCI_HTYPE_ERROR, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&srvhp, OCI_HTYPE_SERVER, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&svchp, OCI_HTYPE_SVCCTX, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&usrhp, OCI_HTYPE_SESSION, 0, NULL);

    // Attach to server
    // printf("Attaching server...\n");
    status = OCIServerAttach(srvhp, errhp, (text *)"cbdcauto_low", strlen("cbdcauto_low"), OCI_DEFAULT);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // Set attribute server context
    // printf("Setting attribute server context...\n");
    status = OCIAttrSet(svchp, OCI_HTYPE_SVCCTX, srvhp, 0, OCI_ATTR_SERVER, errhp);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // Set attribute session context
    // printf("Setting attribute session context...\n");
    status = OCIAttrSet(usrhp, OCI_HTYPE_SESSION, username, strlen(username), OCI_ATTR_USERNAME, errhp);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }
    status = OCIAttrSet(usrhp, OCI_HTYPE_SESSION, password, strlen(password), OCI_ATTR_PASSWORD, errhp);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // Log in
    // printf("Logging in...\n");
    status = OCISessionBegin(svchp, errhp, usrhp, OCI_CRED_RDBMS, OCI_DEFAULT);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }
    status = OCIAttrSet(svchp, OCI_HTYPE_SVCCTX, usrhp, 0, OCI_ATTR_SESSION, errhp);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }

    // Execute SELECT statement
    // printf("Preparing SELECT statement...\n");
    status = OCIHandleAlloc(envhp, (void **)&stmthp, OCI_HTYPE_STMT, 0, NULL);
    if (status != OCI_SUCCESS) {
        print_oci_error(errhp);
        return 1;
    }
    status = OCIStmtPrepare(stmthp, errhp, (text *)"SELECT * FROM test", strlen("SELECT * FROM test"), OCI_NTV_SYNTAX, OCI_DEFAULT);
    if (status != OCI_SUCCESS) {
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
    if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
        print_oci_error(errhp);
        return 1;
    }

    // Print results
    printf("ID\tName\n");
    printf("--------\n");
    while (1) {
        status = OCIStmtFetch(stmthp, errhp, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
        if (status == OCI_NO_DATA) {
            break;
        } else if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
            print_oci_error(errhp);
            return 1;
        }

        printf("%d\t%s\n", id, name);
    }

    // Clean up
    OCIHandleFree(stmthp, OCI_HTYPE_STMT);
    OCISessionEnd(svchp, errhp, usrhp, OCI_DEFAULT);
    OCIServerDetach(srvhp, errhp, OCI_DEFAULT);

    // Free handles
    OCIHandleFree(usrhp, OCI_HTYPE_SESSION);
    OCIHandleFree(svchp, OCI_HTYPE_SVCCTX);
    OCIHandleFree(srvhp, OCI_HTYPE_SERVER);
    OCIHandleFree(errhp, OCI_HTYPE_ERROR);
    OCIHandleFree(envhp, OCI_HTYPE_ENV);

    return 0;
}

void print_oci_error(OCIError *errhp) {
    sb4 errcode = 0;
    text errbuf[512];
    OCIErrorGet((dvoid *)errhp, 1, (text *)NULL, &errcode, errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
    printf("Error %d: %s\n", errcode, errbuf);
}


// gcc -o connect connect.c -I../instantclient/sdk/include -L../instantclient -lclntsh
// instant client is in ../instantclient
// wallet is in ../wallet