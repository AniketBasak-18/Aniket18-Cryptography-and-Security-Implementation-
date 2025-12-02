#include <stdio.h>
#include <stdlib.h>

int main() {                                     // used chatgpt
    FILE *fp;
    char buffer[128];

    // Use Windows-compatible command
    fp = popen("wmic cpu get CurrentClockSpeed", "r");
    if (fp == NULL) {
        perror("Failed to run command");
        exit(EXIT_FAILURE);
    }

    printf("CPU Current Clock Speed (MHz):\n\n");

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("%s", buffer);
    }

    pclose(fp);
    return 0;
}
