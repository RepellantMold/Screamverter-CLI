#include "conv.h"

stx_song_header_t stx_song_header;

u16 stx_inst_pointers[STX_MAXSMP] = {0};
u16 stx_pat_pointers[STX_MAXPAT] = {0};
stx_pcm_parapointers stx_pcm_pointers[STX_MAXSMP] = {0};
static u8 sample_data[USHRT_MAX] = {0};

static void place_pattern_and_ins_parapointers_s3mtostx(internal_state_t* context, stx_parapointers* pointers,
                                                        usize pattern_count, usize sample_count);
static void handle_sample_headers_s3mtostx(internal_state_t* context, usize sample_count);
static void handle_patterns_s3mtostx(internal_state_t* context, usize pattern_count);
static void handle_pattern_and_ins_parapointers_s3mtostx(internal_state_t* context, stx_parapointers* pointers,
                                                         usize pattern_count, usize sample_count);
static void handle_pcm_parapointer_s3mtostx(internal_state_t* context, usize i);
static int handle_pcm_s3mtostx(internal_state_t* context, usize sample_count);

int convert_s3m_to_stx(internal_state_t* context) {
  FILE *S3Mfile = context->infile, *STXfile = context->outfile;
  const bool verbose = context->flags.verbose_mode;
  u8* STXOrders = NULL;
  stx_parapointers parapointers;

  fprintf(stderr, "This feature is not finished!\n");

  if (!S3Mfile || !STXfile)
    return FOC_OPEN_FAILURE;
  if (ferror(S3Mfile) || ferror(STXfile))
    return FOC_MALFORMED_FILE;
  if (check_valid_s3m(S3Mfile))
    return FOC_NOT_S3M_FILE;

  grab_s3m_song_header(S3Mfile);

  context->stats.original_order_count = (u8)s3m_song_header.total_orders;
  context->stats.sample_count = (u8)s3m_song_header.total_instruments;
  if (context->stats.sample_count > STM_MAXSMP)
    fprintf(stderr, "Sample count exceeds 31 (%lu > 31), only using 31.", (u32)context->stats.sample_count);
  context->stats.pattern_count = (u8)s3m_song_header.total_patterns;
  if (context->stats.pattern_count > STM_MAXPAT)
    fprintf(stderr, "Pattern count exceeds 63 (%lu > 63), only converting 63.", (u32)context->stats.pattern_count);

  if (verbose)
    show_s3m_song_header();

  check_s3m_channels();

  convert_song_header_s3mtostx();
  write_stx_song_header(STXfile);

  grab_s3m_orders(S3Mfile);
  grab_s3m_parapointers(S3Mfile);

  place_pattern_and_ins_parapointers_s3mtostx(context, &parapointers, context->stats.pattern_count,
                                              context->stats.sample_count);

  STXOrders = calloc(context->stats.order_count, sizeof(u8) * STX_ORDERMULTIPLIER);
  convert_song_orders_s3mtostx(context->stats.order_count, STXOrders);
  fwrite(STXOrders, sizeof(u8), sizeof(STXOrders), STXfile);
  free(STXOrders);

  handle_sample_headers_s3mtostx(context, context->stats.sample_count);

  handle_patterns_s3mtostx(context, context->stats.pattern_count);

  if (handle_pcm_s3mtostx(context, context->stats.sample_count))
    return FOC_SAMPLE_FAIL;

  handle_pattern_and_ins_parapointers_s3mtostx(context, &parapointers, context->stats.pattern_count,
                                               context->stats.sample_count);

  puts("Conversion done successfully!");
  return FOC_SUCCESS;
}

static void handle_pattern_and_ins_parapointers_s3mtostx(internal_state_t* context, stx_parapointers* pointers,
                                                         usize pattern_count, usize sample_count) {
  FILE* STXfile = context->outfile;

  if (!pointers)
    return;

  fseek(STXfile, pointers->patpara_table_pos << 4, SEEK_SET);
  (void)!fwrite(stx_pat_pointers, sizeof(u16), context->stats.pattern_count, STXfile);
  fseek(STXfile, pointers->inspara_table_pos << 4, SEEK_SET);
  (void)!fwrite(stx_inst_pointers, sizeof(u16), context->stats.sample_count, STXfile);
}

static void place_pattern_and_ins_parapointers_s3mtostx(internal_state_t* context, stx_parapointers* pointers,
                                                        usize pattern_count, usize sample_count) {
  FILE* STXfile = context->outfile;
  usize saved_pos = 0;
  u8 channel_settings[STX_MAXCHN] = {0};

  if (!pointers)
    return;

  pointers->patpara_table_pos = (u16)convert_to_parapointer(ftell(STXfile));
  (void)!fwrite(s3m_pat_pointers, sizeof(u16), context->stats.pattern_count, STXfile);
  pointers->inspara_table_pos = (u16)convert_to_parapointer(ftell(STXfile));
  (void)!fwrite(s3m_inst_pointers, sizeof(u16), context->stats.sample_count, STXfile);
  pointers->chn_table_pos = (u16)convert_to_parapointer(ftell(STXfile));
  (void)!fwrite(channel_settings, sizeof(u8), STX_MAXCHN, STXfile);

  saved_pos = (usize)ftell(STXfile);

  fseek(STXfile, 32, SEEK_SET);
  (void)!fwrite(&pointers->patpara_table_pos, sizeof(u16), 1, STXfile);
  (void)!fwrite(&pointers->inspara_table_pos, sizeof(u16), 1, STXfile);
  (void)!fwrite(&pointers->chn_table_pos, sizeof(u16), 1, STXfile);
  fseek(STXfile, (long)saved_pos, SEEK_SET);
}

static void handle_sample_headers_s3mtostx(internal_state_t* context, usize sample_count) {
  FILE *S3Mfile = context->infile, *STXfile = context->outfile;
  const bool verbose = context->flags.verbose_mode;
  register usize i = 0;

  for (; i < context->stats.sample_count; i++) {
    if (verbose)
      printf("Sample %zu:\n", i);
    grab_s3m_instrument_header_data(S3Mfile, s3m_inst_pointers[i]);
    s3m_pcm_pointers[i] = grab_s3m_pcm_pointer();
    s3m_pcm_lens[i] = grab_s3m_pcm_len();

    if (verbose)
      show_s3m_inst_header();

    /* convert_s3m_instrument_header_s3mtostx(); */

    stx_inst_pointers[i] = (u16)convert_to_parapointer(ftell(STXfile));
    write_stx_instrument_header(STXfile);
  }
}

static void handle_patterns_s3mtostx(internal_state_t* context, usize pattern_count) {
  FILE *S3Mfile = context->infile, *STXfile = context->outfile;
  register usize i = 0;

  for (i = 0; i < context->stats.pattern_count; i++) {
    (void)!printf("Converting pattern %zu...\n", i);
    parse_s3m_pattern(S3Mfile, s3m_pat_pointers[i]);
    stx_pat_pointers[i] = (u16)convert_to_parapointer(ftell(STXfile));
    convert_s3m_pattern_to_stx(STXfile);
    (void)!printf("Pattern %zu written.\n", i);
  }
}

static int handle_pcm_s3mtostx(internal_state_t* context, usize sample_count) {
  FILE *S3Mfile = context->infile, *STXfile = context->outfile;
  register usize i = 0, sample_len = 0, padding_len = 0;
  internal_sample_t sc;

  for (; i < context->stats.sample_count; i++) {
    sample_len = s3m_pcm_lens[i];

    if (!sample_len)
      continue;

    sc.length = sample_len;

    padding_len = calculate_sample_padding(sc);

    sc.pcm = sample_data;

    (void)!printf("Converting sample %lu...\n", (u32)i);

    if (dump_sample_data(S3Mfile, s3m_pcm_pointers[i], &sc))
      return FOC_SAMPLE_FAIL;

    pcm_swap_sign(&sc);

    if (!padding_len)
      goto dontaddpadding;

    sample_len += padding_len;

  dontaddpadding:
    handle_pcm_parapointer_s3mtostx(context, i);

    (void)!fwrite(sample_data, sizeof(u8), sample_len, STXfile);

    (void)!printf("Sample %lu written.\n", (u32)i);
  }

  return FOC_SUCCESS;
}

static void handle_pcm_parapointer_s3mtostx(internal_state_t* context, usize i) {
  FILE* outfile = context->outfile;
  const usize saved_pos = (usize)ftell(outfile),
              header_pos = 64 + (context->stats.pattern_count * 2) + ((80 * (i + 1)) - 67);

  stx_pcm_pointers[i] = calculate_stx_sample_parapointer();

  (void)!fseek(outfile, (long)header_pos, SEEK_SET);

  fputc(stx_pcm_pointers[i].memseg.bytes.high, outfile);
  fputc(stx_pcm_pointers[i].memseg.bytes.low1, outfile);
  fputc(stx_pcm_pointers[i].memseg.bytes.low2, outfile);

  (void)!fseek(outfile, (long)saved_pos, SEEK_SET);
}
