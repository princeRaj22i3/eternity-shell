#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char *buffer = NULL;
size_t buff_size = 0;
ssize_t text;

int main(){
    system("clear");
    printf("eternity-shell $ ");

    text = getline(&buffer, &buff_size, stdin);

    if(text == -1){
        perror("Error reading input or EOF reached.\n");
        free(buffer);
        return 1;
    }

    if(text>0 && buffer[text-1] == '\n') buffer[text-1] = '\0';

    printf("%s", buffer);
    free(buffer);
    return 0;
}