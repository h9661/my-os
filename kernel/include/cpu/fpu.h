#ifndef FPU_H
#define FPU_H

#include "../common/types.h"

/* FPU control word bits */
#define FPU_CW_PRECISION_MASK   0x0300
#define FPU_CW_PRECISION_24     0x0000  /* 24-bit precision */
#define FPU_CW_PRECISION_53     0x0200  /* 53-bit precision */
#define FPU_CW_PRECISION_64     0x0300  /* 64-bit precision */

#define FPU_CW_ROUNDING_MASK    0x0C00
#define FPU_CW_ROUND_NEAREST    0x0000  /* Round to nearest */
#define FPU_CW_ROUND_DOWN       0x0400  /* Round down */
#define FPU_CW_ROUND_UP         0x0800  /* Round up */
#define FPU_CW_ROUND_ZERO       0x0C00  /* Round toward zero */

#define FPU_CW_INFINITY         0x1000  /* Infinity control */
#define FPU_CW_PRECISION_EXC    0x0020  /* Precision exception */
#define FPU_CW_UNDERFLOW_EXC    0x0010  /* Underflow exception */
#define FPU_CW_OVERFLOW_EXC     0x0008  /* Overflow exception */
#define FPU_CW_ZERODIV_EXC      0x0004  /* Zero divide exception */
#define FPU_CW_DENORMAL_EXC     0x0002  /* Denormalized operand exception */
#define FPU_CW_INVALID_EXC      0x0001  /* Invalid operation exception */

/* Default FPU control word: mask all exceptions, 64-bit precision, round to nearest */
#define FPU_CW_DEFAULT          (FPU_CW_PRECISION_64 | FPU_CW_ROUND_NEAREST | \
                                 FPU_CW_PRECISION_EXC | FPU_CW_UNDERFLOW_EXC | \
                                 FPU_CW_OVERFLOW_EXC | FPU_CW_ZERODIV_EXC | \
                                 FPU_CW_DENORMAL_EXC | FPU_CW_INVALID_EXC)

/* CR0 register bits for FPU control */
#define CR0_EM_BIT              0x04    /* Emulation bit - set to disable FPU */
#define CR0_TS_BIT              0x08    /* Task Switch bit - set by CPU on task switch */
#define CR0_MP_BIT              0x02    /* Monitor coProcessor bit */
#define CR0_NE_BIT              0x20    /* Numeric Error bit - enable native FPU error reporting */

/* Function prototypes */
void fpu_initialize(void);
bool fpu_is_available(void);
void fpu_enable(void);
void fpu_disable(void);
void fpu_clear_exceptions(void);
uint16_t fpu_get_control_word(void);
void fpu_set_control_word(uint16_t cw);
uint16_t fpu_get_status_word(void);
void fpu_save_state(void* state);
void fpu_restore_state(void* state);

#endif /* FPU_H */
