#include <raylib.h>
#include <stdio.h>

struct address {
        unsigned int b : 12;
};

int main() {
    // reserve memory
    address MEM[4096];

    // Starting addresses from 000 to 1FF are empty
    address *PC = MEM + 0x200;

    return 0;
}
