/*
 * Screamverter
 * Made by RepellantMold/cs127 under ISC license
 *
 * Written in ANSI C
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse/optparse.h"

#include "envcheck.h"
#include "ext.h"

#include "main.h"

#include "conv.h"

u8 original_order_count = 0, order_count = 0, sample_count = 0, pattern_count = 0;

internal_state_t main_context;

void print_help(void) {
  puts("Usage: screamverter [options] -i<inputfile> -o<outputfile>");
  puts("(C) RepellantMold/cs127, 2024");
  puts("Licensed under ISC");
  puts("");
  puts("Options:");
  puts("  -h, --help       Print this help and exit");
  puts("  -v, --verbose    Enable extremely verbose output");
  puts("  -s, --sanitize   Sanitize sample names during conversion, useful for saving them on DOS");
  puts("  -m, --memory     Handle effects calling effect memory (helps with Scream Tracker 2.24 and below)");
  puts("  -2, --stm        Convert the S3M to STM (default)");
  puts("  -x, --stx        Convert the S3M to STX (unfinished)");
  puts("  -i, --input      Input file");
  puts("  -o, --output     Output file");
}

s3m_song_header_t s3m_song_header;
s3m_instrument_header_t s3m_inst_header;

u8 s3m_order_array[S3M_ORDER_LIST_SIZE] = {S3M_ORDER_END};
u16 s3m_inst_pointers[S3M_MAXSMP] = {0};
u16 s3m_pat_pointers[S3M_MAXPAT] = {0};
u32 s3m_pcm_pointers[S3M_MAXSMP] = {0};
u16 s3m_pcm_lens[S3M_MAXSMP] = {0};
u16 s3m_cwtv;

static char input_filename[4096], output_filename[4096];

int check_valid_s3m(FILE* S3Mfile);

int convert_s3m_to_stm(internal_state_t* context);
int convert_s3m_to_stx(internal_state_t* context);

int main(int argc, char* argv[]) {
  register int return_value = FOC_SUCCESS;
  usize conversion_type = FOC_S3MTOSTM;
  FILE *outfile = NULL, *infile = NULL;

  int option;
  struct optparse options;
  struct optparse_long longopts[] = {
      {"sanitize", 's', OPTPARSE_NONE},  {"verbose", 'v', OPTPARSE_NONE},    {"memory", 'm', OPTPARSE_NONE},
      {"help", 'h', OPTPARSE_NONE},      {"stm", '2', OPTPARSE_NONE},        {"stx", 'x', OPTPARSE_NONE},
      {"input", 'i', OPTPARSE_REQUIRED}, {"output", 'o', OPTPARSE_REQUIRED}, {0}};

  if (argc < 2) {
  print_da_help:
    print_help();
    return FOC_NO_FILENAMES;
  }

  optparse_init(&options, argv);
  while ((option = optparse_long(&options, longopts, NULL)) != -1) {
    switch (option) {
      case 'v': main_context.flags.verbose_mode = true; break;
      case 's': main_context.flags.sanitize_sample_names = true; break;
      case 'm': main_context.flags.handle_effect_memory = true; break;
      case '2': conversion_type = FOC_S3MTOSTM; break;
      case 'x': conversion_type = FOC_S3MTOSTX; break;
      case 'i': strncpy(input_filename, options.optarg, sizeof(input_filename)); break;
      case 'o': strncpy(output_filename, options.optarg, sizeof(output_filename)); break;
      case 'h': goto print_da_help; break;
      case '?': fprintf(stderr, "%s: %s\n", argv[0], options.errmsg); exit(EXIT_FAILURE);
    }
  }

  infile = fopen(input_filename, "rb");
  outfile = fopen(output_filename, "wb");

  if (!infile || !outfile) {
    return_value |= FOC_OPEN_FAILURE;
    perror("Failed to open file");
    exit(EXIT_FAILURE);
  }

  main_context.infile = infile;
  main_context.outfile = outfile;
  main_context.conversion_type = conversion_type;

  srand((u32)time(0));

  switch (conversion_type) {
    case FOC_S3MTOSTM: return_value |= convert_s3m_to_stm(&main_context); break;
    case FOC_S3MTOSTX: return_value |= convert_s3m_to_stx(&main_context); break;
  }

  fclose(infile);
  fclose(outfile);

  return return_value;
}

int check_valid_s3m(FILE* S3Mfile) {
  char scrm[4] = {0};

  if (!S3Mfile)
    return FOC_MALFORMED_FILE;

  (void)!fseek(S3Mfile, 44, SEEK_SET);

  if (fread(scrm, sizeof(char), 4, S3Mfile) != 4) {
    print_error("Failed to read S3M header!");
    return FOC_MALFORMED_FILE;
  };

  if (memcmp(scrm, "SCRM", 4)) {
    print_error("This is not an S3M file!");
    return FOC_NOT_S3M_FILE;
  }

  rewind(S3Mfile);
  return FOC_SUCCESS;
}
