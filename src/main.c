#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h> 


int main(){
    system("clear");

    while(1){
        printf("eternity-shell $ ");

        // getline variables
        char *buffer = NULL;
        size_t buff_size = 0;
        ssize_t text;
        // I/O Redirection variables
        char* operation = NULL;
        char* file = NULL;

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
    
        //exec argument vector
        char *args[text+1];
        int pipeFound = 0;
        int i = 0;

        //Tokenisation
        char *token = strtok(buffer, " ");
        while(token != NULL && i<text){
            if(strcmp(token,">")==0 || strcmp(token,">>")==0 || strcmp(token,"<")==0){
                operation = token;
                file = strtok(NULL, " ");
                break;
            }
            if(strcmp(token,"|")==0){
                pipeFound = 1;
            }
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        //pipes
        int r = 0;
        char *commands[10][10];
        if(pipeFound){
            int c = 0;
            int j = 0;

            while(args[j] != NULL){
                if(strcmp(args[j],"|") == 0){
                    commands[r][c] = NULL;
                    r++;
                    c=0;
                    j++;
                    continue;
                }
                commands[r][c++] = args[j];
                j++;
            }
            commands[r][c] = NULL;
        }

        //Exit
        if(strcmp(args[0],"exit") == 0){
            free(buffer);
            exit(EXIT_SUCCESS);
        }

        // Change Directory 
        if(strcmp(args[0],"cd") == 0){
            if(chdir(args[1]) != 0){
                perror("eternity-shell: cd");
            }
            free(buffer);
            continue;
        }

        int cnt = 0;
        int prev_pipe = -1;

        for(cnt = 0; cnt<=r; cnt++){
            
            int fd[2];
            if(cnt<r) {
                pipe(fd);
            }

            // fork syetem call
            pid_t id = fork();

            if(id < 0){
                perror("Fork failed");
                exit(EXIT_FAILURE);
            }
            // Child Process
            else if(id == 0){
                if(operation){
                    if(!file){
                        fprintf(stderr, "eternity-shell: Unexpected syntax error. File name not given.\n");
                        _exit(EXIT_FAILURE);
                    }
                    // I/O Redirection
                    if(strcmp(operation,">")==0){
                        int file_fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if(file_fd == -1){
                            fprintf(stderr, "eternity-shell: %s ", file);
                            perror("");
                            _exit(EXIT_FAILURE);
                        }
                        if(dup2(file_fd, 1) < 0){
                            perror("eternity-shell: redirection failed");
                            close(file_fd);
                            _exit(EXIT_FAILURE);
                        }
                        close(file_fd);
                    }
                    else if(strcmp(operation,">>")==0){
                        int file_fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if(file_fd == -1){
                            fprintf(stderr, "eternity-shell: %s ", file);
                            perror("");
                            _exit(EXIT_FAILURE);
                        }
                        if(dup2(file_fd, 1) < 0){
                            perror("eternity-shell: redirection failed");
                            close(file_fd);
                            _exit(EXIT_FAILURE);
                        }
                        close(file_fd);
                    }
                    else{
                        int file_fd = open(file, O_RDONLY, 0644);
                        if(file_fd == -1){
                            perror("File does not exist");
                            _exit(EXIT_FAILURE);
                        }
                        if(dup2(file_fd, 0) < 0){
                            perror("eternity-shell: redirection failed");
                            close(file_fd);
                            _exit(EXIT_FAILURE);
                        }
                        close(file_fd);
                    }
                }

                if(pipeFound){
                    if(prev_pipe != -1){
                        dup2(prev_pipe,0);
                        close(prev_pipe);
                    }
                    if(cnt<r){
                        close(fd[0]);
                        dup2(fd[1],1);
                        close(fd[1]);
                    }
                    execvp(commands[cnt][0], commands[cnt]);
                }
                // exec system call
                execvp(args[0], args);
                perror("Exec failed");
                _exit(EXIT_FAILURE);
            }

            //Parent Process
            else{
                if(pipeFound){
                    if(prev_pipe != -1){
                        close(prev_pipe);
                    }
                    if(cnt<r){
                        close(fd[1]);
                        prev_pipe = fd[0];
                    }
                }
                else{
                    int status;
                    waitpid(id, &status, 0);
                }
            }
        }

        for(cnt = 0; cnt<=r; cnt++){
            wait(NULL);
        }
        free(buffer);
    }

    return 0;
}