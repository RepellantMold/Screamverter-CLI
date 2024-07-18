# Screamverter

[![standard-readme compliant](https://img.shields.io/badge/standard--readme-OK-green.svg?style=plastic)](https://github.com/RichardLitt/standard-readme)

A command line tool to downgrade (that being a **very heavy keyword**) Scream Tracker 3 modules to Scream Tracker 2 modules, written in ANSI C.

## Table of Contents

- [Compiling](#compiling)
- [Usage](#usage)
- [Maintainers](#maintainers)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

## Compiling

This uses the CMake build system, but it should not be too difficult to compile yourself.
This repo uses a submodule for the internals (libtrkcvt), so make sure to run `git submodule update --init` before you attempt to compile!

## Usage

```sh
screamverter [options] -i<inputfile> -o<outputfile>

# Options:
#   -h, --help       Print this help and exit
#   -v, --verbose    Enable extremely verbose output
#   -s, --sanitize   Sanitize sample names during conversion, useful for saving them on DOS
#   -m, --memory     Handle effects calling effect memory (helps with Scream Tracker 2.24 and below)
#   -2, --stm        Convert the S3M to STM (default)
#   -x, --stx        Convert the S3M to STX (unfinished)
#   -i, --input      Input file
#   -o, --output     Output file
```

> [!IMPORTANT]
> These points are important for a decent conversion.
>
> - 4 channels preferably (it only converts the first four channels.)
> - Sample sizes cannot exceed 65535 (and samples cannot be in 16-bit format.)
> - No panning (Scream Tracker 2/Scream Tracker Music Interface Kit is mono) or Adlib.
> - Sample names are gonna truncated heavily to 12 characters if its file name field is blank.
> - You can only use effects A - J, with some caveats with the table shown below.
> - This converter strips out all end markers on the order list, which may not be desired unless you add a `Bxx` to the last pattern you intend for the listener.
>
> | Effect | Function in ST2/STMIK - in ST3/others | Notes/Quirks (Effect memory **does not exist** in Scream Tracker 2, so a parameter of 0 will act like a no-op for any effect that normally has it. (e.g. volume slide, portamento slides, vibrato, etc.)) |
> |---:|:---:|---|
> | `Axx` | Set speed - Set ticks per row | It has a scaling factor (TODO: somehow get tempo to approximate in said scaling factor) alongside setting ticks per row . |
> | `Bxx` | Set next order - Position jump | It does not perform an immediate pattern break unlike most other trackers, so it's best to place it alongside a Pattern break. |
> | `Cxx` | Pattern break | The parameter is ignored which is identical behavior to NoiseTracker. |
> | `Dxy` | Volume slide | There's no fine slides, and `y` will take priority if both `x` and `y` are specified. |
> | `Exx` | Portamento down | This effect can cause the note period to underflow, and there are no fine/extra-fine slides. |
> | `Fxx` | Portamento up | This effect can cause the note period to overflow, ditto for fine/extra-fine slides. |
> | `Gxx` | Tone portamento | It does not reset the sample volume unlike most other trackers. |
> | `Hxy` | Vibrato | The depth is doubled compared to other trackers, this converter will attempt to adjust the depth accordingly. |
> | `Ixy` | Tremor | A parameter of 0 will be a very fast tremor, like old Scream Tracker 3. |
> | `Jxy` | Arpeggio | The effect behaves strangely since it tends to skip to y halfway through a row if `x` was 0, and just implemented strangely in general. |
> | `Kxx` | (No-op) | This can be entered into the editor but it does not do anything, **it also bugs out Scream Tracker 3's importer so it's best to not use it!** |
> | `Lxx` | (No-op) | Ditto. |
> | `Mxx` | (No-op) | Ditto. |
> | `Nxx` | (No-op) | Ditto. |
> | `Oxx` | (No-op) | Ditto. |

## Maintainers

[@RepellantMold](https://github.com/RepellantMold)
[@cs127](https://github.com/cs127)

## Contributing

PRs accepted.

Small note: If editing the README, please conform to the
[standard-readme](https://github.com/RichardLitt/standard-readme) specification.

## License

[ISC](LICENSE) © 2024 RepellantMold

## Acknowledgements

- Future Crew: [Scream Tracker 2](https://github.com/libxmp/libxmp/blob/master/docs/formats/stm.txt) and [3](http://www.textfiles.com/programming/FORMATS/s3m-form.txt) specifications
- [Claudio Matsuoka: unofficial Scream Tracker Music Interface Kit format specifications](https://github.com/libxmp/libxmp/blob/master/docs/formats/stx-format.txt)
- skeeto - [optparse, the argument parser used](https://github.com/skeeto/optparse)
