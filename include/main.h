#ifndef __MAIN_H
#define __MAIN_H
#include <stdio.h>
#include <stdlib.h>

#include "ext.h"
#include "struct.h"

#include "fmt/s3m.h"
#include "fmt/stm.h"
#include "fmt/stx.h"

extern internal_state_t main_context;

enum FOC_ConversionMode { FOC_S3MTOSTM = 0x00, FOC_S3MTOSTX = 0x01 };

#endif
