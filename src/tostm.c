#include "conv.h"

stm_song_header_t stm_song_header;

static u8 sample_data[USHRT_MAX] = {0};
u8 stm_order_list[STM_ORDER_LIST_SIZE] = {STM_ORDER_END};

static int handle_pcm_s3mtostm(internal_state_t* context);
static void handle_sample_headers_s3mtostm(internal_state_t* context, stm_instrument_header_t* stm_instrument_header);
static void handle_patterns_s3mtostm(internal_state_t* context);

int convert_s3m_to_stm(internal_state_t* context) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;
  const u8 sample_count = context->stats.sample_count;
  const u8 pattern_count = context->stats.pattern_count;
  if (!S3Mfile || !STMfile)
    return FOC_OPEN_FAILURE;
  if (ferror(S3Mfile) || ferror(STMfile))
    return FOC_MALFORMED_FILE;
  if (check_valid_s3m(S3Mfile))
    return FOC_NOT_S3M_FILE;

  grab_s3m_song_header(S3Mfile);

  if (sample_count > STM_MAXSMP)
    fprintf(stderr, "Sample count exceeds 31 (%u > 31), only using 31.\n", sample_count);
  context->stats.pattern_count = (u8)s3m_song_header.total_patterns;
  if (pattern_count > STM_MAXPAT)
    fprintf(stderr, "Pattern count exceeds 63 (%u > 63), only converting 63.\n", pattern_count);

  convert_song_header_s3mtostm();
  write_stm_song_header(STMfile);

  grab_s3m_orders(S3Mfile);
  grab_s3m_parapointers(S3Mfile);

  handle_sample_headers_s3mtostm(context, stm_song_header.instruments);

  convert_song_orders_s3mtostm(context->stats.original_order_count);
  fwrite(stm_order_list, sizeof(u8), sizeof(stm_order_list), STMfile);

  handle_patterns_s3mtostm(context);

  if (handle_pcm_s3mtostm(context))
    return FOC_SAMPLE_FAIL;

  puts("Conversion done successfully!");
  return FOC_SUCCESS;
}

static void handle_sample_headers_s3mtostm(internal_state_t* context, stm_instrument_header_t* stm_instrument_header) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;
  const bool verbose = context->flags.verbose_mode;
  register usize i = 0;

  for (; i < STM_MAXSMP; i++) {
    if (i >= context->stats.sample_count) {
      generate_blank_stm_instrument(&stm_instrument_header[i]);
      goto skiptowritingsampleheader;
    }

    if (verbose)
      printf("Sample %lu:\n", (u32)i);

    grab_s3m_instrument_header_data(S3Mfile, s3m_inst_pointers[i]);
    s3m_pcm_pointers[i] = grab_s3m_pcm_pointer();
    s3m_pcm_lens[i] = grab_s3m_pcm_len();

    if (verbose)
      show_s3m_inst_header();

    convert_s3m_instrument_header_s3mtostm(&stm_instrument_header[i]);

  skiptowritingsampleheader:
    write_stm_instrument_header(STMfile, &stm_instrument_header[i]);
  }
}

static void handle_patterns_s3mtostm(internal_state_t* context) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;
  register usize i = 0;

  for (i = 0; i < STM_MAXPAT; i++) {
    if (i >= context->stats.pattern_count)
      break;
    printf("Converting pattern %lu...\n", (u32)i);
    parse_s3m_pattern(S3Mfile, s3m_pat_pointers[i]);
    stm_pattern_t pattern;
    convert_s3m_pattern_to_stm(&pattern);
    fwrite(&pattern, STM_PATSIZE, 1, STMfile);
    printf("Pattern %lu written.\n", (u32)i);
  }
}

static int handle_pcm_s3mtostm(internal_state_t* context) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;
  register usize i = 0, sample_len = 0, padding_len = 0;
  internal_sample_t sc;

  u16 stm_pcm_pointers[STM_MAXSMP] = {0};

  for (; i < STM_MAXSMP; i++) {
    if (i >= context->stats.sample_count)
      break;

    sample_len = s3m_pcm_lens[i];

    if (!sample_len)
      continue;

    sc.length = sample_len;

    padding_len = (u16)calculate_sample_padding(sc);

    sc.pcm = sample_data;

    printf("Converting sample %lu...\n", (u32)i);

    if (dump_sample_data(S3Mfile, s3m_pcm_pointers[i], &sc))
      return FOC_SAMPLE_FAIL;

    pcm_swap_sign(&sc);

    if (!padding_len)
      goto dontaddpadding;

    sample_len += padding_len;

  dontaddpadding:
    const usize saved_pos = (usize)ftell(context->outfile), header_pos = 48 + ((32 * (i + 1)) - 18);

    stm_pcm_pointers[i] = calculate_stm_sample_parapointer();

    (void)!fseek(context->outfile, (long)header_pos, SEEK_SET);

    fputw(stm_pcm_pointers[i], context->outfile);

    (void)!fseek(context->outfile, (long)saved_pos, SEEK_SET);

    (void)!fwrite(sample_data, sizeof(u8), sample_len, STMfile);
    (void)!printf("Sample %lu written.\n", (u32)i);
  }

  return FOC_SUCCESS;
}