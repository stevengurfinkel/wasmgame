#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        return EXIT_FAILURE;
    }

    printf("let %s = [\n", argv[1]);
    int c = '\0';
    int pos = 0;
    while ((c = getchar()) != EOF) {
        if (pos != 0) {
            printf(",");
            if (pos % 16 == 0) {
                printf(" // %04X - %04X\n", pos - 16, pos - 1);
            }
        }
        printf("0x%02X", c);
        pos++;
    }
    printf("];\n");
    return 0;
}
