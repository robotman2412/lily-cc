
#include <asm_postproc.h>
#include <objects.h>
#include <stdio.h>

void output_logisim_hex(asm_ctx_t *ctx) {
	FILE *loop = fopen("/tmp/lily-cc-tmp-bin", "w+");
	FILE *real = ctx->out_fd;
	ctx->out_fd = loop;
	output_native(ctx);
	
}
