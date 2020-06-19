#ifndef ASM_H
#define ASM_H
#include "buffer.h"

#ifdef WIN32
#define MAX_CALL_ARGS 4
#else
#define MAX_CALL_ARGS 6
#endif
#define MAX_GP_REGS 8

void asm_generator (buffer_t *buffer, FILE *outfile);

#endif /* ifndef ASM_H */
