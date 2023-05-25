#include <stdio.h>

int main() {
    char buffer_[] = "v1 := t2";
    char tmp_str1[20], tmp_str2[20], tmp_str3[20];

    int result = sscanf(buffer_, "%s := &%s + %s", tmp_str1, tmp_str2, tmp_str3);

    if (result == 3) {
        printf("String parsed successfully.\n");
        printf("tmp_str1: %s\n", tmp_str1);
        printf("tmp_str2: %s\n", tmp_str2);
        printf("tmp_str3: %s\n", tmp_str3);
    } else {
        printf("Invalid input.\n");
    }

    return 0;
}
