### potential ideas

- output version 1.0 or 1.1 STX files based on command line option (default to 1.1?)


### todo

- [ ] warn when a file is too large
- [ ] add a channel remap system if there's muted channels, it would prevent someone from muting the first 4 channels and breaking the conversion
- [ ] work on stx conversion (and switch based on file extension of the output file)
- [ ] work on converting the S3M tempo to be scaling factor in STM

### in progress

### complete

- [x] give verbose option
- [x] song header conversion
- [x] work on sign conversion with the samples
- [x] have the order conversion discard markers
- [x] work on pattern conversion
- [x] sample header conversion (the parapointers at the end of the header; how do we handle them?)