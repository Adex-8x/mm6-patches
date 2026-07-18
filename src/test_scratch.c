#include <pmdsky.h>
#include <cot.h>

/*
* Linker is configured to dump the compiled binary into an external
* you can then take the hex representation and pass it as a string
* to the special process SpLoadCode
* The loaded code can be executed like any other menu using the
* special menu ID
*/

// Example variable declaration
__attribute__((section(".data.scratch"))) int example = 0;

// Main menu functions
__attribute__((section(".text.scratch"), used)) void CreateMenu() {
    example = 1;
}
__attribute__((section(".text.scratch"), used)) void CloseMenu() {
    example = 2;
}
__attribute__((section(".text.scratch"), used)) bool UpdateMenu() {
    example = 0;
    return true;
}

__attribute__((section(".text.scratchmain"), naked)) void entrypoint() {
    asm volatile("b CreateMenu");
    asm volatile("b CloseMenu");
    asm volatile("b UpdateMenu");
}