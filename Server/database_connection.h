#ifndef DATABASE_CONNECTION_H
#define DATABASE_CONNECTION_H

#include <oci.h>

int initialize(OCIEnv **envhp, OCIServer **srvhp, OCISession **usrhp, OCISvcCtx **svchp, OCIError **errhp, sword *status);
void print_oci_error(OCIError *errhp);
int read_key_file(char *username, char *password, char *wallet_pw);
void clean_up(OCISession **usrhp, OCISvcCtx **svchp, OCIServer **srvhp, OCIError **errhp, OCIEnv **envhp);
int set_environment();
char* execute_sql_query(sword status, OCIEnv *envhp, OCISvcCtx *svchp, OCIError *errhp, OCIServer *srvhp, OCISession *usrhp, const char *post_data);
void connect_to_db(sword status, OCIEnv *envhp, OCIServer **srvhp, OCISession **usrhp, OCISvcCtx **svchp, OCIError **errhp, const char *username, const char *password);
void disconnect_from_db(OCISession *usrhp, OCISvcCtx *svchp, OCIServer *srvhp, OCIError *errhp, OCIEnv *envhp);

#endif // DATABASE_CONNECTION_H
