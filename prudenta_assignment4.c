#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_BACKGROUND_PROCESSES 10

int child_status = 0;
int background_process[MAX_BACKGROUND_PROCESSES];
bool background_enabled = true;
bool status_previous_command = false;

void status()
{

    if(WIFEXITED(child_status)){

        printf("exit value %d\n", WEXITSTATUS(child_status));
        fflush(stdout);
    
    } else {

        printf("terminated by signal %d\n", WTERMSIG(child_status));
        fflush(stdout);

    }

}

/*
* Code adapted from:
* Title: sample_parser.c
* Author: Oregon State University 
* Date 2/25/2025
* Availability: https://canvas.oregonstate.edu/courses/1987883/files/109834045?wrap=1
*/

#define INPUT_LENGTH 2048
#define MAX_ARGS	 512


struct command_line
{

    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;

};
 
 
struct command_line *parse_input() 
{

    char input[INPUT_LENGTH];
    struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));
 
    // Get input
    printf(": ");
    fflush(stdout);
    fgets(input, INPUT_LENGTH, stdin);
 
    // Tokenize the input
    char *token = strtok(input, " \n");
    while(token) {
        if(!strcmp(token,"<")) {

            curr_command->input_file = strdup(strtok(NULL," \n"));

        } else if(!strcmp(token,">")) {

             curr_command->output_file = strdup(strtok(NULL," \n"));

        } else if(!strcmp(token,"&")) {

             curr_command->is_bg = true;

        } else {

             curr_command->argv[curr_command->argc++] = strdup(token);

        }

         token=strtok(NULL," \n");

    }

    return curr_command;

}
 
void cd(struct command_line *curr_command)
{   

    char curr_abs_directory[INPUT_LENGTH];
    char curr_rel_directory[INPUT_LENGTH];
    DIR* opened_directory;
    struct dirent *entry;

    strcpy(curr_abs_directory, getenv("PWD"));
    strcpy(curr_rel_directory, strrchr(curr_abs_directory, '/'));

    // CD with no arguments handling
    if(curr_command->argc == 1) {

        setenv("PWD", getenv("HOME"), 1);
        chdir(getenv("HOME"));
    
    } else if(strcmp(curr_command->argv[1], ".") != 0) {

        // Relative path handling with dots
        if(strcmp(curr_command->argv[1], "..") == 0 || (strcmp(curr_command->argv[1], "../") == 0)) {
            
            curr_abs_directory[strlen(curr_abs_directory) - strlen(curr_rel_directory)] = '\0';

            setenv("PWD", curr_abs_directory, 1);
            chdir(curr_abs_directory);
        
        // Absolute path handling
        } else if(opendir(curr_command->argv[1]) != NULL) {

            setenv("PWD", curr_command->argv[1], 1);
            chdir(curr_command->argv[1]);
        
        // Relative path handling with directory name
        } else {

            opened_directory = opendir(getenv("PWD"));

            while((entry = readdir(opened_directory)) != NULL) {

                if(strcmp(entry->d_name, curr_command->argv[1]) == 0) {
                    
                    strcat(curr_abs_directory, "/");
                    strcat(curr_abs_directory, curr_command->argv[1]);
                    setenv("PWD", curr_abs_directory, 1);
                    chdir(curr_abs_directory);

                }

            }

            closedir(opened_directory);
        
        }

    }

}

/*
* Code adapted from:
* Title: Using exec() with fork() example, Signal Handling API, and Using dup2() for Redirection Example
* Author: Oregon State University 
* Date 2/26/2025
* Availability: 
* https://canvas.oregonstate.edu/courses/1987883/pages/exploration-process-api-executing-a-new-program?module_item_id=24956220
* https://canvas.oregonstate.edu/courses/1987883/pages/exploration-signal-handling-api?module_item_id=24956227
* https://canvas.oregonstate.edu/courses/1987883/pages/exploration-processes-and-i-slash-o?module_item_id=24956228
*/

void handle_SIGTSTP(int signo)
{   

    if(background_enabled == true) {

        write(STDOUT_FILENO, "\nEntering foreground-only mode (& is now ignored)\n: ", 52);
        background_enabled = false;

    } else {

        write(STDOUT_FILENO, "\nExiting foreground-only mode\n: ", 32);
        background_enabled = true;

    }

}

void other_command(struct command_line *curr_command)
{
    
    // All children ignore STGTSP signal
    struct sigaction ignore_action = {0};

    ignore_action.sa_handler = SIG_IGN;

    sigaction(SIGTSTP, &ignore_action, NULL);

    pid_t spawn_pid = fork();

    switch(spawn_pid) {
        case -1:
            perror("fork()\n");
            exit(1);
            break;

        case 0:
            // IO Redirection
            // Input file
            if(curr_command->input_file != NULL) {

                int input_file = open(curr_command->input_file, O_RDONLY, 0640);

                if(input_file == -1) {

                    perror("open()");
                    exit(1);

                }

                int input_result = dup2(input_file, 0);

                if(input_result == -1) {

                    perror("dup2()");
                    exit(2);

                }

            }

            // Output file
            if(curr_command->output_file != NULL) {

                int output_file = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                
                if(output_file == -1) {

                    perror("open()");
                    exit(1);

                }

                int output_result = dup2(output_file, 1);

                if(output_result == -1) {

                    perror("dup2()");
                    exit(2);

                }

            }

            // Signal Handling
            // Foreground processes do not ignore SIGINT signal;
            if(curr_command->is_bg == false) {

                struct sigaction SIGINT_action = {0};

                SIGINT_action.sa_handler = SIG_DFL;
                sigfillset(&SIGINT_action.sa_mask);
                SIGINT_action.sa_flags = 0;
                sigaction(SIGINT, &SIGINT_action, NULL);

            }

            // exec()
            execvp(curr_command->argv[0], curr_command->argv);
            perror("execvp");
            exit(2);

            break;

        default:
            if(curr_command->is_bg == false || background_enabled == false) {

                waitpid(spawn_pid, &child_status, 0);
            
            } else {
                
                printf("background pid is %d\n", spawn_pid);
                fflush(stdout);
                
                int i = 0;
                int current_process = background_process[i];

                while(current_process != 0 && i < MAX_BACKGROUND_PROCESSES) {
                    
                    i++;
                    current_process = background_process[i];

                }

                background_process[i] = spawn_pid;
    
            }

            break;  
    }

}

void cleanup_background_processes() {

    for(int i=0; i < MAX_BACKGROUND_PROCESSES; i++) {

        if(background_process[i] != 0) {

            kill(background_process[i], SIGTERM);

        }

    }

}

int main()
{
    
    struct command_line *curr_command;
    pid_t spawn_pid;
    int background_child_status;
    struct sigaction ignore_action = {0}, SIGTSTP_action = {0};

    while(true)
    {

        ignore_action.sa_handler = SIG_IGN;
        sigaction(SIGINT, &ignore_action, NULL);

        SIGTSTP_action.sa_handler = handle_SIGTSTP;
        sigfillset(&SIGTSTP_action.sa_mask);
        SIGTSTP_action.sa_flags = SA_RESTART;
        sigaction(SIGTSTP, &SIGTSTP_action, NULL);     

        for(int i=0; i<MAX_BACKGROUND_PROCESSES; i++) {

            spawn_pid = background_process[i];

            if(waitpid(spawn_pid, &background_child_status, WNOHANG) != 0 && spawn_pid != 0) {

                background_process[i] = 0; 

                if(child_status == 0) {

                    printf("background pid %d is done: exit value %d\n", spawn_pid, background_child_status);

                } else {

                    printf("background pid %d is done: terminated by signal %d\n", spawn_pid, background_child_status);

                }

            }

        }

        if (WIFSIGNALED(child_status) && status_previous_command == false) {

            status();

        }   

        curr_command = parse_input();

        if(curr_command->argc != 0 && curr_command->argv[0][0] != '#') {

            if(strcmp(curr_command->argv[0], "cd") == 0) {

                status_previous_command = false;
                cd(curr_command);

            } else if(strcmp(curr_command->argv[0], "exit") == 0) {

                cleanup_background_processes();
                break;

            } else if(strcmp(curr_command->argv[0], "status") == 0) {

                status_previous_command = true;
                status(child_status);

            } else {

                status_previous_command = false;
                other_command(curr_command);

            }

        }

        free(curr_command);

    }

    return EXIT_SUCCESS;

}