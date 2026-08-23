#include "include/lammp/version.h"
#include <stdio.h>

int main(void) {
    printf("Hello, LMMP! \n");
    printf("LMMP version: %s\n", lmmp_get_version());
    printf("LMMP build type: %s\n", lmmp_get_build_type());
    return 0;
}
