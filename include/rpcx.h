#ifndef RPCX_H_
#define	RPCX_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct RPCXInfo RPCXInfo;
struct RPCXInfo {
	int fd;
	
	int			bpp;
	int			planes;
	bool		valid;
	
	uint8_t		palette[256*3];
	int		colors;

	int		w;
	int		h;

	unsigned char	*data;
};

RPCXInfo *rpcx_init(const char *fname);
int rpcx_read(RPCXInfo *ri);
int rpcx_close(RPCXInfo *ri)

#endif
