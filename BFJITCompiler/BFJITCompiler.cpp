// BFJITCompiler.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <Windows.h>

typedef void (__stdcall *barebonesBytecode)();
//https://defuse.ca/online-x86-assembler.htm#disassembly Don't forget to check x64

void __stdcall displayOneCharacter(int charToShow);

int main(int argc, char** argv)
{
    BYTE programMemory[256];
    memset(programMemory, 0, 256);
    BYTE* bytecode = reinterpret_cast<BYTE*>(VirtualAlloc(NULL, 65536, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    BYTE* currentPos = bytecode;

    void (__stdcall *dispFuncPtr)(int) = displayOneCharacter;
    displayOneCharacter('B');
    displayOneCharacter('F');
    displayOneCharacter(':');

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
                //On rentre dans un bloc (potentiellement d'une seule instruction) arithmétique, on move la valeur pointée par rdx
                *(DWORD*)(currentPos) = 0x00028B48; //Little-endian reversed of "mov rax, [rdx]"
                currentPos += 3;
            }
        }
        else if(wasAddSubInstruction) //On sort d'un bloc arithmétique
        {
            *(DWORD*)(currentPos) = 0x00028948; //Little-endian reversed of "mov [rdx], rax"
            currentPos += 3;
        }

        switch (currentReadSourceChar)
        {
        case '>':
            *(DWORD*)(currentPos) = 0x01C28348; //Little-endian reversed of "add rdx,0x1"
            currentPos += sizeof(DWORD);
            break;
        case '<':
            *(DWORD*)(currentPos) = 0x01EA8348; //Little-endian reversed of "sub rdx,0x1"
            currentPos += sizeof(DWORD);
            break;
        case '+':
            *(DWORD*)(currentPos) = 0x01C08348; //Little-endian reversed of "add rax,0x1"
            currentPos += sizeof(DWORD);
            break;
        case '-':
            *(DWORD*)(currentPos) = 0x01E88348; //Little-endian reversed of "sub rax,0x1"
            currentPos += sizeof(DWORD);
            break;
        case '.':
            //We save RDX beforehand
            /*
            
            
            
            
            TODO: Faire la sauvegarde dans une variable locale pas affectée
            
            
            
            
            
            */
            *(DWORD*)currentPos = 0x00D18949; // mov r9,rdx
            currentPos += 3;
            *(DWORD*)currentPos = 0x000A8B48; //mov rcx, [rdx]
            currentPos += 3;
            *(short*)currentPos = 0xB848; //Beginning of mov rax, ADDRESS
            currentPos += sizeof(short);
            *(void**)currentPos = (void*)dispFuncPtr; //The actual ADDRESS of putchar
            currentPos += sizeof(void**);
            *(short*)currentPos = 0xD0FF; //call rax
            currentPos += sizeof(short);
            //We restore edx
            *(DWORD*)currentPos = 0x00CA894C; // mov rdx,r9
            currentPos += 3;
            break;
        case ',':
            break;
        case '[':
            break;
        case ']':
            break;
        }
    }

    //Ajout d'un ret à la fin du programme
    *currentPos = '\xC3';

    barebonesBytecode bytecodeFunc = reinterpret_cast<barebonesBytecode>(bytecode);
    bytecodeFunc();

    //Exécution
    

    fclose(sourceFile);
    VirtualFree(bytecode, NULL, MEM_RELEASE);
    return 0;
}

void __stdcall displayOneCharacter(int charToShow)
{
    putchar(charToShow & 0xFF); //On ne garde que les bits peu significatifs
    return;
}
