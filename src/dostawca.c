#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Brak argumentu!\n");
        return 1;
    }
    
    printf("Dostawca skladnika %s: Start\n", argv[1]);
    return 0;
}