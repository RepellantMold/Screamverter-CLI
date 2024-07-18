#ifndef __CONV_H
#define __CONV_H
#include "fmt/s3m.h"
#include "fmt/stm.h"
#include "fmt/stx.h"

#include "ext.h"
#include "file.h"

#include "parapnt.h"
#include "pattern.h"
#include "sample.h"
#include "sample_header.h"
#include "song_header.h"
#include "struct.h"

#include "main.h"


void convert_song_header_s3mtostm(void);
void convert_song_header_s3mtostx(void);

extern int check_valid_s3m(FILE* S3Mfile);

#endif
