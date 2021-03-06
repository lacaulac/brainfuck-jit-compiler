# Brainfuck JIT compiler

*This project is inactive.*

This was an attempt at making an x64 Brainfuck just-in-time compiler.
The "kinda" part comes from the fact that the compiler isn't quite capable of compiling on the fly at the moment and instead compiles the code before actually starting to run any of it. There are two reasons for that:
- Brainfuck doesn't support functions. This means almost the whole code is going to run at some point.
- This was my first try at making a compiler and I didn't plan to get much deeper in the subject than dealing with compiling code and basic optimisation.

## How does it work?

The program works by performing the following three tasks:

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
  - As the compiled code was written to an executable part of the compiler's memory, we can just run it as a function.
  - Also, just free memory and close the handle we've opened to the source code file
