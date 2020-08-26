# Brainfuck JIT compiler

This is a half-baked attempt at making a x64 brainfuck kinda-just-in-time compiler.
The "kinda" part comes from the fact that the compiler isn't quite capable of compiling on the fly at the moment and instead does compile the code before actually running it. The reason for that is that this the first time I'm writing something that's somewhat close to a compiler (hence the choice of the Brainfuck language) and I didn't want to over-complicate the task at first, since I'd really like to be able to make it capable of real JIT compiling.

## How does it work?

The program works by performing this three tasks:

- Initialising the compiler & running environment (Getting ready)
  - Allocating writable & executable memory for the compiled code to be written to
  - Checking if a valid file name was given to the compiler
  - Opening the source code file
  - Inserting the code responsible for writing the brainfuck data-bank memory address into the `rdx` register, which will allow the compiled code to access it.
- Compiling the code (Yay!)
  - Stepping through the BF code and translating every instruction into its x64 "equivalent" (the choice of registers was completely arbitrary)
  - Kind of optimising the way addition and substraction work by not loading and saving the currently selected value from the data-bank to the arithmetic register if not needed.
  - Adding a `ret` instruction at the end, in order to return back to the `main` function
- Running the code (The most straight-forward part)
  -  As the compiled code was written to an executable part of the compiler's memory, we can just run it as a function.
  - Also, just free memory and close the handle we've opened to the source code file

## What's left to do

### Features

The following BF language features are missing:

- `[` & `]` loops

### Optimisation

- Arithmetic operations chaining
  - Make it so that only one add or sub instruction is used if multiple arithmetic BF instructions are chained
- Character displaying
  - Simplify the way a character is displayed, as a lot of CPU cycles are used due to bug fixes