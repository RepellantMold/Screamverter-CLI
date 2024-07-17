#include "conv.h"

stm_song_header_t stm_song_header;

static u8 sample_data[USHRT_MAX] = {0};
u8 stm_order_list[STM_ORDER_LIST_SIZE] = {STM_ORDER_END};
u8 stm_pattern[64][4][4] = {{{0xFF, 0x01, 0x80, 0x00}}};

u16 stm_pcm_pointers[STM_MAXSMP] = {0};

static int handle_pcm_s3mtostm(internal_state_t* context, usize sample_count);
static void handle_pcm_parapointer_s3mtostm(internal_state_t* context, usize i);
static void handle_sample_headers_s3mtostm(internal_state_t* context, usize sample_count,
                                           stm_instrument_header_t* stm_instrument_header);
static void handle_patterns_s3mtostm(internal_state_t* context, usize pattern_count);

int convert_s3m_to_stm(internal_state_t* context) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;

  if (!S3Mfile || !STMfile)
    return FOC_OPEN_FAILURE;
  if (ferror(S3Mfile) || ferror(STMfile))
    return FOC_MALFORMED_FILE;
  if (check_valid_s3m(S3Mfile))
    return FOC_NOT_S3M_FILE;

  grab_s3m_song_header(S3Mfile);

  if (sample_count > STM_MAXSMP)
    print_warning("Sample count exceeds 31 (%u > 31), only using 31.", sample_count);
  pattern_count = (u8)s3m_song_header.total_patterns;
  if (pattern_count > STM_MAXPAT)
    print_warning("Pattern count exceeds 63 (%u > 63), only converting 63.", pattern_count);

  convert_song_header_s3mtostm();
  write_stm_song_header(STMfile);

  grab_s3m_orders(S3Mfile);
  grab_s3m_parapointers(S3Mfile);

  handle_sample_headers_s3mtostm(context, sample_count, stm_song_header.instruments);

  convert_song_orders_s3mtostm(order_count);
  fwrite(stm_order_list, sizeof(u8), sizeof(stm_order_list), STMfile);

  handle_patterns_s3mtostm(context, pattern_count);

  if (handle_pcm_s3mtostm(context, sample_count))
    return FOC_SAMPLE_FAIL;

  puts("Conversion done successfully!");
  return FOC_SUCCESS;
}

static void handle_sample_headers_s3mtostm(internal_state_t* context, usize sample_count,
                                           stm_instrument_header_t* stm_instrument_header) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;
  const bool verbose = context->flags.verbose_mode;
  register usize i = 0;

  for (; i < STM_MAXSMP; i++) {
    if (i >= sample_count) {
      generate_blank_stm_instrument(&stm_instrument_header[i]);
      goto skiptowritingsampleheader;
    }

    if (verbose)
      printf("Sample %zu:\n", i);

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

static void handle_patterns_s3mtostm(internal_state_t* context, usize pattern_count) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;
  register usize i = 0;

  for (i = 0; i < STM_MAXPAT; i++) {
    if (i >= pattern_count)
      break;
    printf("Converting pattern %zu...\n", i);
    parse_s3m_pattern(S3Mfile, s3m_pat_pointers[i]);
    convert_s3m_pattern_to_stm();
    fwrite(stm_pattern, STM_PATSIZE, 1, STMfile);
    printf("Pattern %zu written.\n", i);
  }
}

static int handle_pcm_s3mtostm(internal_state_t* context, usize sample_count) {
  FILE *S3Mfile = context->infile, *STMfile = context->outfile;
  register usize i = 0, sample_len = 0, padding_len = 0;
  internal_sample_t sc;

  for (; i < STM_MAXSMP; i++) {
    if (i >= sample_count)
      break;

    sample_len = s3m_pcm_lens[i];

    if (!sample_len)
      continue;

    padding_len = (u16)calculate_sample_padding(sample_len);

    sc.length = sample_len;
    sc.pcm = sample_data;

    printf("Converting sample %zu...\n", i);

    if (dump_sample_data(S3Mfile, s3m_pcm_pointers[i], &sc))
      return FOC_SAMPLE_FAIL;

    pcm_swap_sign(&sc);

    if (!padding_len)
      goto dontaddpadding;

    sample_len += padding_len;

  dontaddpadding:
    handle_pcm_parapointer_s3mtostm(context, i);

    (void)!fwrite(sample_data, sizeof(u8), sample_len, STMfile);
    (void)!printf("Sample %zu written.\n", i);
  }

  return FOC_SUCCESS;
}

static void handle_pcm_parapointer_s3mtostm(internal_state_t* context, usize i) {
  const usize saved_pos = (usize)ftell(context->outfile), header_pos = 48 + ((32 * (i + 1)) - 18);

  stm_pcm_pointers[i] = calculate_stm_sample_parapointer();

  (void)!fseek(context->outfile, (long)header_pos, SEEK_SET);

  fputw(stm_pcm_pointers[i], context->outfile);

  (void)!fseek(context->outfile, (long)saved_pos, SEEK_SET);
}
