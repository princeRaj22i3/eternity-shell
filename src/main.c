#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


int main(){
    system("clear");

    while(1){
        printf("eternity-shell $ ");

        char *buffer = NULL;
        size_t buff_size = 0;
        ssize_t text;

        text = getline(&buffer, &buff_size, stdin);

        if(text == -1){
            if(feof(stdin)){
                printf("EOF detected. Exiting eternity-shell...\n");
                free(buffer);
                exit(EXIT_SUCCESS);
            }
            if(ferror(stdin)){
                perror("eternity-shell: Critical read error");
                free(buffer);
                exit(EXIT_FAILURE);
            }
        }

        if(text>0 && buffer[text-1] == '\n') buffer[text-1] = '\0';
        if(buffer[0]=='\0'){
            free(buffer);
            continue;
        }
    
        char *args[text+1];
        int i = 0;

        char *token = strtok(buffer, " ");
        while(token != NULL && i<text){
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if(strcmp(args[0],"exit") == 0){
            free(buffer);
            exit(EXIT_SUCCESS);
        }

        if(strcmp(args[0],"cd") == 0){
            if(chdir(args[1]) != 0){
                perror("eternity-shell: cd");
            }
            free(buffer);
            continue;
        }

        pid_t id = fork();
        if(id < 0){
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        else if(id == 0){
            execvp(args[0], args);
            perror("Exec failed");
            exit(EXIT_FAILURE);
        }
        else{
            int status;
            waitpid(id, &status, 0);
        }

        free(buffer);
    }

    return 0;
}