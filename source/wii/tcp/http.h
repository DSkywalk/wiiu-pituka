#ifndef _HTTP_H_
#define _HTTP_H_

#include <gctypes.h>

typedef enum {
	HTTPR_OK,
	HTTPR_ERR_CONNECT,
	HTTPR_ERR_REQUEST,
	HTTPR_ERR_STATUS,
	HTTPR_ERR_TOOBIG,
	HTTPR_ERR_RECEIVE
} http_res;

int http_request(const char *url, FILE *hfile, u8 *buffer, u32 maxsize);

#endif

