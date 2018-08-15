[![Build Status](https://travis-ci.org/nh13/ksw.svg?branch=master)](https://travis-ci.org/nh13/ksw)
[![License](http://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/nh13/ksw/blob/master/LICENSE)
[![Language](http://img.shields.io/badge/language-c-brightgreen.svg)](http://devdocs.io/c/)

# Ksw: (interactive) smith-waterman in C

## <a name="overview"></a>Overview

This is a light-weight adaptation of [klib](https://github.com/attractivechaos/klib) to facilitate various types of [Smith-Waterman](https://en.wikipedia.org/wiki/Smith%E2%80%93Waterman_algorithm) alignment, but more importantly do so interactively.

The following four alignment modes are supported:

* Local alignment: a sub-sequence of the query aligned to a sub-sequence of the target
* Glocal: the full query aligned to a sub-sequence of the target
* Extension - a prefix of the query aligned to a prefix of the target
* Global - full query aligned to the full target

## <a name="running"></a>Running

The utility reads the query and target sequences from standard input, so running it without anything on standard input will cause it to never exit.
The query should be on the first line, while the target on the second line.
Subsequent lines should alternate between query and target.

This tool can be run **_interactively_**.  Meaning, the tool reads in one query and target at a time, runs alignment, then writes the alignment output, then waits for more input.
Therefore, we can wrap this tool in a process that writes to the standard input of this tool, waits for the alignment result on standard output, then does something else, then feeds more data to standard input. 


## <a name="installation"></a>Installation

Clone this repository:

```
git clone --recursive git@github.com:nh13/ksw.git
```
Note the use of --recursive. This is required in order to download all nested git submodules for external repositories.


To get version displayed in the utility, do the following:

```
cp post-commit .git/hooks/post-commit;
chmod +x .git/hooks/post-commit;
```

Finally, to compile, type:

```
make
```

The executable is named `ksw`.

To run tests, type:

```
make test
```

To install to `/usr/local/bin`, type:

```
make install
```

For a custom location (ex. `~/local/bin`), type:

```
make install PREFIX=~/local/bin
```
