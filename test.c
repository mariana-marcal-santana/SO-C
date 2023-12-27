#include <stdio.h>
#include <string.h>

int main() {

    char str1[] = "Hello world";
    char str2[] = ",\0how\0are\0you?";

    size_t total_length = sizeof(str1) + sizeof(str2) - 1;

    char result[total_length];

    memcpy(result, str1, sizeof(str1));

    memcpy(result + sizeof(str1) - 1, str2, sizeof(str2) - 1);

    printf("Result: %s\n", result);
    printf("Character by character: ");
    int i = 0;
    for (i = 0; i < sizeof(result); ++i) {
        printf("%c", result[i]);
    }
    printf("\n");

    return 0;
}