#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <signal.h>

pid_t background_pids[100];
int bid = 0;
void shift_back(int *id){
    int i = 0;
    while(i < bid-1){
        *id = *(id+1);
        id++;
        i++;
    }
}
void handle_sigchld(int sig){
    for(int i=0; i<bid; i++){
        int status;
        int res = waitpid(background_pids[i], &status, WNOHANG);
        if(res > 0){   
            printf("\nThe child process with pid, [%d], has finished executing\n", background_pids[i]);
            shift_back(&background_pids[i]);
            bid--;
            i--;
        }
    }
}

int main(){
    system("clear");
    signal(SIGCHLD, handle_sigchld);

    while(1){
        printf("eternity-shell $ ");

        // getline variables
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
    
        //exec argument vector
        char *args[text+1];
        int i = 0;

        //Tokenisation
        char truncate[2] = ">";
        char input[2] = "<";
        char append[3] = ">>";
        char pipe_ch[2] = "|";
        char ampersand[2] = "&";
        
        char *sp_ch_ptr = NULL;

        char *src = buffer;
        char *dest = buffer;
        
        while(*src != '\0'){
            
            while(*src == ' '){
                src++;
            }
            if(*src == '\0') break;
            dest = src;
            
            if(*dest == '"' && *(dest+1) == '"'){
                dest++;
                src++;
            }
            if(*dest != '&') args[i++] = dest;
            
            int in_quotes = 0;
            
            while(*src != '\0'){
                
                if(*src == '"'){
                    in_quotes = !in_quotes;
                    src++;
                }
                if(!in_quotes && *src == ' '){
                    break;
                }
                if(!in_quotes && (*src == '<' || *src == '>' || *src == '|' || *src == '&')){
                    if(*src == '>' && *(src+1)=='>'){
                        sp_ch_ptr = append;
                        *(src+1) = ' ';
                    }
                    else if(*src == '>'){
                        sp_ch_ptr = truncate;
                    }
                    else if(*src == '<'){
                        sp_ch_ptr = input;
                    }
                    else if(*src == '|'){
                        sp_ch_ptr = pipe_ch;
                    }
                    else if(*src == '&'){
                        sp_ch_ptr = ampersand;
                    }
                    args[i++] = sp_ch_ptr;
                    break;
                }
                
                if(*src != '"') *dest = *src;
                dest++;
                src++;
            }
            
            *dest = '\0';
            src++;
        }
        
        args[i] = NULL;

        //background processes
        int background_process = 0;

        //pipes
        char *commands[10][10];
        int r = 0;
        int c = 0;

        // I/O redirection
        char* operation[10];
        char* file[10];
        int redirect_command_index[10];
        int o = 0;

        int j = 0;
        int current_command = 0;

        while(args[j] != NULL){
            if(strcmp(args[j],"&") == 0){
                background_process = 1;
                args[j] = NULL;
                break;
            }
            if(strcmp(args[j],"|") == 0){
                commands[r][c] = NULL;
                r++;
                c=0;
                j++;
                current_command++;
                continue;
            }
            if(strcmp(args[j],">")==0 || strcmp(args[j],">>")==0 || strcmp(args[j],"<")==0){
                operation[o] = args[j];
                file[o] = args[j+1];
                redirect_command_index[o] = current_command;
                o++;
                j += 2;
                continue;
            }
            commands[r][c++] = args[j];
            j++;
        }
        commands[r][c] = NULL;


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

        //Pipe variables
        int cnt = 0;
        int prev_pipe = -1;

        //foreground pids
        int foreground_pids[10];
        int fid = 0;

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

                if(prev_pipe != -1){
                    dup2(prev_pipe,0);
                    close(prev_pipe);
                }
                if(cnt<r){
                    close(fd[0]);
                    dup2(fd[1],1);
                    close(fd[1]);
                }
    

                // I/O Redirection
                for(int x=0; x<o; x++){
                    if(redirect_command_index[x]!=cnt) continue;

                    if(strcmp(operation[x],">")==0){
                        int file_fd = open(file[x], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if(file_fd == -1){
                            fprintf(stderr, "eternity-shell: %s ", file[x]);
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
                    else if(strcmp(operation[x],">>")==0){
                        int file_fd = open(file[x], O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if(file_fd == -1){
                            fprintf(stderr, "eternity-shell: %s ", file[x]);
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
                        int file_fd = open(file[x], O_RDONLY, 0644);
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

                execvp(commands[cnt][0], commands[cnt]);
                // exec system call
                execvp(args[0], args);
                perror("Exec failed");
                _exit(EXIT_FAILURE);
            }

            //Parent Process
            else{                
                if(background_process) 
                    background_pids[bid++] = id;
                else 
                    foreground_pids[fid++] = id;

                if(prev_pipe != -1){
                    close(prev_pipe);
                }
                if(cnt<r){
                    close(fd[1]);
                    prev_pipe = fd[0];
                }
            }
        }

        while(fid>0){
            int status;
            waitpid(foreground_pids[--fid], &status, 0);
        }

        free(buffer);
    }

    return 0;
}