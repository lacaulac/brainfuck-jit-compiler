// BFJITCompiler.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <stack>
#include <Windows.h>

typedef void (__cdecl *barebonesBytecode)();
//https://defuse.ca/online-x86-assembler.htm#disassembly Don't forget to check x64

//A wrapper around putchar
void __stdcall displayOneCharacter(int charToShow);

//A wrapper around getchar
int __stdcall getOneCharacter();

void preFunctionCall(BYTE** pCurrentPos);
void postFunctionCall(BYTE** pCurrentPo);

int main(int argc, char** argv)
{
    //The data-bank of the brainfuck program
    BYTE* programMemory = reinterpret_cast<BYTE*>(VirtualAlloc(NULL, 16777216, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    memset(programMemory, 0, 65536);

    //The memory location where the processor instructions will be stored and executed
    BYTE* bytecode = reinterpret_cast<BYTE*>(VirtualAlloc(NULL, 16777216, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    //The current position when compiling
    BYTE* currentPos = bytecode;

    //Stores the instruction adresses of non-yet written/defined end-of-loop pointers
    std::stack<BYTE**> loopStack;


    void (__stdcall *dispFuncPtr)(int) = displayOneCharacter;
    int (__stdcall *getCFuncPtr)() = getOneCharacter;

    //Let's set rdx to programMemory's address
    *currentPos = 0x48;
    currentPos++;
    *currentPos = 0xBA;
    currentPos++;

    *(BYTE**)(currentPos) = programMemory;
    currentPos += sizeof(BYTE*);

    if (argc != 2)
    {
        printf("Usage: %s <source file>\n", argv[0]);
        return -1;
    }

    FILE* sourceFile;
    fopen_s(&sourceFile, argv[1], "r");
    if (sourceFile == NULL)
    {
        printf("Couldn't open %s.\n", argv[1]);
        return -2;
    }

    char oldChar = 'F';
    char currentReadSourceChar = 'F';
    while (currentReadSourceChar != EOF)
    {
        oldChar = currentReadSourceChar;
        currentReadSourceChar = fgetc(sourceFile);
        bool isAddSubInstruction = currentReadSourceChar == '+' || currentReadSourceChar == '-';
        bool wasAddSubInstruction = oldChar == '+' || oldChar == '-';

        if (isAddSubInstruction)
        {
            if (!wasAddSubInstruction)
            {
                //Entering an "arithmetic" block (even though it might only contain one arithmetic instruction), so we get the value pointed by rdx into rax in order to do the math stuff
                *(DWORD*)(currentPos) = 0x00028B48; //Little-endian reversed of "mov rax, [rdx]"
                currentPos += 3;
            }
        }
        else if(wasAddSubInstruction) //Leaving an "arithmetic" block
        {
            *(DWORD*)(currentPos) = 0x00028948; //Little-endian reversed of "mov [rdx], rax"
            currentPos += 3;
        }

        switch (currentReadSourceChar)
        {
        case '>':
            if (oldChar == '>' && *(currentPos - 1) != 0x7F) //If the last operation was an addition and the addition amount is less than 0x7F
            {
                *(currentPos - 1) += 1;
            }
            else
            {
                *(DWORD*)(currentPos) = 0x01C28348; //Little-endian reversed of "add rdx,0x1"
                currentPos += sizeof(DWORD);
            }
            break;
        case '<':
            if (oldChar == '<' && *(currentPos - 1) != 0x7F) //If the last operation was an addition and the addition amount is less than 0x7F
            {
                *(currentPos - 1) += 1;
            }
            else
            {
                *(DWORD*)(currentPos) = 0x01EA8348; //Little-endian reversed of "sub rdx,0x1"
                currentPos += sizeof(DWORD);
            }
            break;
        case '+':
            if (oldChar == '+' && *(currentPos - 1) != 0x7F) //If the last operation was an addition and the addition amount is less than 0x7F
            {
                *(currentPos - 1) += 1;
            }
            else
            {
                *(DWORD*)(currentPos) = 0x01C08348; //Little-endian reversed of "add rax,0x1"
                currentPos += sizeof(DWORD);
            }
            break;
        case '-':
            if (oldChar == '-' && *(currentPos - 1) != 0x7F) //If the last operation was an addition and the addition amount is less than 0x7F
            {
                *(currentPos - 1) += 1;
            }
            else
            {
                *(DWORD*)(currentPos) = 0x01E88348; //Little-endian reversed of "sub rax,0x1"
                currentPos += sizeof(DWORD);
            }
            break;
        case '.':
            preFunctionCall(&currentPos);

            //We do the function calling stuff
            *(DWORD*)currentPos = 0x000A8B48; //mov rcx, [rdx]
            currentPos += 3;
            *(short*)currentPos = 0xB848; //Beginning of mov rax, ADDRESS
            currentPos += sizeof(short);
            *(void**)currentPos = (void*)dispFuncPtr; //The actual ADDRESS of displayOneCharacter
            currentPos += sizeof(void**);
            *(short*)currentPos = 0xD0FF; //call rax
            currentPos += sizeof(short);

            postFunctionCall(&currentPos);

            break;
        case ',':
            preFunctionCall(&currentPos);

            *(short*)currentPos = 0xB848; //Beginning of mov rax, ADDRESS
            currentPos += sizeof(short);
            *(void**)currentPos = (void*)getCFuncPtr; //The actual ADDRESS of getOneCharacter
            currentPos += sizeof(void**);
            *(short*)currentPos = 0xD0FF; //call rax
            currentPos += sizeof(short);

            //Can't keep the return value in rax since it's used by postFunctionCall to restore edx
            *(DWORD*)currentPos = 0x00C28949; //mov r10, rax
            currentPos += 3;

            postFunctionCall(&currentPos);

            *(DWORD*)currentPos = 0x0012894C; //mov [rdx], r10
            currentPos += 3;

            break;
        case '[':
            //Add the "while" code
            *(DWORD*)(currentPos) = 0x00C03148; //xor rax, rax
            currentPos += 3;

            *(short*)currentPos = 0x028A; //mov al, [rdx]
            currentPos += sizeof(short);

            *(short*)currentPos = 0xC084; //test al, al
            currentPos += sizeof(short);

            *(short*)currentPos = 0x0C75; //jnz atadbitfurther
            currentPos += sizeof(short);

            *(short*)currentPos = 0xB848; //Beginning of mov rax, ADDRESS
            currentPos += sizeof(short);

            loopStack.push(reinterpret_cast<BYTE**>(currentPos)); //Let's store the address where we should write the end of loop's address whenever we know it

            *(BYTE***)currentPos = reinterpret_cast<BYTE**>(0xAAAAAAAAAAAAAAAA); //A temporary adress for the after-loop
            currentPos += sizeof(BYTE***);

            *(short*)currentPos = 0xE0FF; //jmp rax
            currentPos += sizeof(short);

            break;
        case ']':
            if (!loopStack.empty()) //If there's unclosed loops in the loop stack
            {
                BYTE** loopStartPosition = loopStack.top(); //Get the last added loop beginning location
                loopStack.pop(); //Remove it from the loop stack

                //Let's jump back to the zero-equality checking code

                *(short*)currentPos = 0xB848; //Beginning of mov rax, ADDRESS
                currentPos += sizeof(short);

                *(BYTE**)currentPos = reinterpret_cast<BYTE*>(loopStartPosition) - 11; //A temporary adress for the after-loop
                currentPos += sizeof(BYTE**);

                *(short*)currentPos = 0xE0FF; //jmp rax
                currentPos += sizeof(short);

                *loopStartPosition = currentPos;
            }
            break;
        }
    }

    //Adding a `ret` instruction at the end to get back to executing main
    *currentPos = '\xC3';

    //Close the source code file
    fclose(sourceFile);

    //Execution

    printf("The compiled code is %d bytes long.\nNow executing...\n\n", (currentPos - bytecode) + 1);


    barebonesBytecode bytecodeFunc = reinterpret_cast<barebonesBytecode>(bytecode);
    bytecodeFunc();

    //Free the allocated memory
    VirtualFree(programMemory, NULL, MEM_RELEASE);
    VirtualFree(bytecode, NULL, MEM_RELEASE);


    return 0;
}

void __stdcall displayOneCharacter(int charToShow)
{
    putchar(charToShow & 0xFF); //Only keep the least significant byte (because ASCII, duh)
    return;
}

int __stdcall getOneCharacter()
{
    return getchar() & 0xFF;
}

void preFunctionCall(BYTE** pCurrentPos)
{
    //We save RDX beforehand
    **pCurrentPos = 0x52; //push rdx
    *pCurrentPos += 1;

    //Let's move the stack around to avoid any bad stuff happening (or else the call wrecks the stack and thus the call stack for some reason)
    **(void***)pCurrentPos = (void*)0x00000000ffec8148; //sub rsp,0xff
    *pCurrentPos += 7;
    **(void***)pCurrentPos = (void*)0x00000000ffed8148; //sub rbp,0xff
    *pCurrentPos += 7;
}

void postFunctionCall(BYTE** pCurrentPos)
{
    //Let's move back the stack in it's normal place
    **(void***)pCurrentPos = (void*)0x00000000ffc48148; //add rsp,0xff
    *pCurrentPos += 7;
    **(void***)pCurrentPos = (void*)0x00000000ffc58148; //add rbp,0xff
    *pCurrentPos += 7;

    
    //We restore rdx
    **pCurrentPos = 0x5A; //push rdx
    *pCurrentPos += 1;
}
