# MIPS-Simulator
C++ Simulator for MIPS assembly language

## Build executable
```bash
make
```
This builds the executable ./simulator

## Run
```bash
./simulator [file_name] [mode] [ROW_ACCESS_DELAY] [COL_ACCESS_DELAY]
```
Here, 4 command line arguments are taken.

## Usage
* Build the executable
* Store the input file in the ./test folder, for example - test/test_sample.asm
* Sample:
```bash
./simulator test_sample.asm fast 10 2
```
This simulates the file test_sample.asm in Non Blocking mode (Part 2) with ROW_ACCESS_DELAY = 10 and COL_ACCESS_DELAY = 2
```bash
./simulator test1.asm slow 5 3
```
This simulates the file test1.asm in Blocking mode (Part 1) with ROW_ACCESS_DELAY = 5 and COL_ACCESS_DELAY = 3

## Clean
To remove executables, use
```bash
make clean
```
