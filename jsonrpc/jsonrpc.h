#include "jansson.h"

#define JSONRPC_PARSE_ERROR -32700
#define JSONRPC_INVALID_REQUEST -32600
#define JSONRPC_METHOD_NOT_FOUND -32601
#define JSONRPC_INVALID_PARAMS -32602
#define JSONRPC_INTERNAL_ERROR -32603
#define JSONRPC_MTK_ILLEGAL_STATE           7
#define JSONRPC_MTK_ILLEGAL_ARG             3
#define JSONRPC_MTK_CANT_OPERATE            90000
#define JSONRPC_MTK_REQ_RETRY               40000
#define JSONRPC_MTK_CLIENT_OVER             40001
#define JSONRPC_MTK_COUNT_OVER              41801
#define JSONRPC_MTK_INDEX_OVER              41802
#define JSONRPC_MTK_UNSUPPORT_FORMAT        41803
#define JSONRPC_MTK_NOT_FOUND               41804
#define JSONRPC_MTK_REQ_DUP                 40003
#define JSONRPC_MTK_WRONG_UUID              41806
#define JSONRPC_MTK_WRONG_NICKNAME          41807
#define JSONRPC_MTK_NOT_SELECTED            41808
#define JSONRPC_MTK_FILE_SIZE_ERR           41809

typedef int (*jsonrpc_method_prototype)(json_t *json_params, json_t **result);
struct jsonrpc_method_entry_t
{
	const char *name;
	jsonrpc_method_prototype funcptr;
	const char *params_spec;
};
char *jsonrpc_handler(const char *input, size_t input_len, struct jsonrpc_method_entry_t method_table[]);

json_t *jsonrpc_error_object(int code, json_t *data);
