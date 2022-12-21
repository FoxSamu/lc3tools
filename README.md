# LC3 Toolkit
A simple toolkit for handling assembly and machine code for the Little Computer 3 (LC3) architecture. Written in C++.

It currently supports:
- Partly decompiling LC3 machine code.

# How to use
There are no pre-compiled binaries, but compiling should be fairly simple. No external libraries are being used. Make sure you have `g++` installed and it's up to date to support the C++17 standard.

## Compiling

### On UNIX-like systems
Clone the repository, navigate to the directory in a terminal and type the following:
```bash
./compile
```

This should compile the binary tools into a subdirectory `build`. If permissions are denied to execute the compile script, type the following:
```bash
chmod +x compile
```
And then try again; or run `sh compile`.

Alternatively, you can run the compiler yourself, see the compile script.

### On Windows systems
You'll have to run the compiler yourself, since I have no batch script for this. If you have `g++`, the instructions should be pretty much the same as in the provided compile script.

## Installing
You may wanna move the binary to a location that is on the path. On most Linux systems, you can copy the built files to `/usr/local/bin`:
```bash
sudo cp build/lc3c /usr/local/bin
```

## Running
Type `lc3c -h` to get a help menu.

The program expects a file that has a hexadecimal number on each line (empty lines are ignored). Suppose this file is `assembly.hex`, then you can run the program with `lc3c assembly.hex` and it will print a detailed table converting the hexadecimal machine code into readable assembly, and binary code. To only print the assembly, use the `-a` flag.

Use `-` as input file to use the system input. In this case, an empty line will stop the program.

Note that the assembly output is cannot be assembled as proper LC3 assembly, because it does not create labels. It instead prints raw program counter offsets.