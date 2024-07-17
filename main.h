#ifndef __MAIN_H
#define __MAIN_H
#include <stdio.h>
#include <stdlib.h>

#include "ext.h"

#include "struct.h"
#include "fmt/s3m.h"
#include "fmt/stm.h"
#include "fmt/stx.h"

extern u8 original_order_count, order_count, sample_count, pattern_count;

/* RM: stealing cs127's NTCheck's return values! */
enum FOC_ReturnCode {
  FOC_SUCCESS = 0x00,
  FOC_OPEN_FAILURE = 0x01,
  FOC_NOT_S3M_FILE = 0x02,
  FOC_MALFORMED_FILE = 0x04,
  FOC_CONV_FAILURE = 0x08,
  FOC_ALLOC_FAIL = 0x10,
  FOC_MALFORMED_BUFFER = 0x20,
  FOC_NO_FILENAMES = 0x40,
  FOC_SAMPLE_FAIL = 0x80
};

extern internal_state_t main_context;

void eprintf(const char* format, ...);
void eputs(const char* msg);

void optional_printf(const char* format, ...);
void optional_puts(const char* msg);

void warning_puts(const char* msg);
void warning_printf(const char* format, ...);

#endif
