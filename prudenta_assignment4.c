#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define MAX_BACKGROUND_PROCESSES 10
int child_status = 0;
int background_process[MAX_BACKGROUND_PROCESSES];


void status()
{

    printf("exit value %d\n", child_status);
    fflush(stdout);

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
* Title: Using exec() with fork() example & Custom Handler for SIGINT
* Author: Oregon State University 
* Date 2/26/2025
* Availability: 
* https://canvas.oregonstate.edu/courses/1987883/pages/exploration-process-api-executing-a-new-program?module_item_id=24956220
* https://canvas.oregonstate.edu/courses/1987883/pages/exploration-signal-handling-api?module_item_id=24956227
*/

void handle_SIGCHLD(int signo)
{   


}

void other_command(struct command_line *curr_command)
{

    pid_t spawn_pid = fork();

    switch(spawn_pid) {
        case -1:
            perror("fork()\n");
            exit(1);
            break;

        case 0:
            execvp(curr_command->argv[0], curr_command->argv);
            perror("execvp");
            exit(2);
            break;

        default:
            if(curr_command->is_bg == false) {

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

int main()
{
    
    struct command_line *curr_command;
    pid_t spawn_pid;
    struct sigaction SIGCHLD_action = {0};

    //SIGCHLD_action.sa_handler = handle_SIGCHLD;
    //sigfillset(&SIGCHLD_action.sa_mask);
    //SIGCHLD_action.sa_flags = 0;
    //sigaction(SIGCHLD, &SIGCHLD_action, NULL);

    while(true)
    {

        for(int i=0; i<MAX_BACKGROUND_PROCESSES; i++) {

            spawn_pid = background_process[i];

            if(waitpid(spawn_pid, &child_status, WNOHANG) != 0 && spawn_pid != 0) {

                background_process[i] = 0; 

                if(child_status == 0) {

                    printf("background pid %d is done: exit value %d\n", spawn_pid, child_status);

                } else {

                    printf("background pid %d is done: terminated by signal %d\n", spawn_pid, child_status);

                }

            }

        }

        curr_command = parse_input();
        
        if(curr_command->argc != 0) {

            if(strcmp(curr_command->argv[0], "cd") == 0) {

                cd(curr_command);

            } else if(strcmp(curr_command->argv[0], "exit") == 0) {

                exit(0);

            } else if(strcmp(curr_command->argv[0], "status") == 0) {

                status(child_status);

            } else if(strcmp(curr_command->argv[0], "#") != 0) {

                other_command(curr_command);

            }

        }

        free(curr_command);

    }

    return EXIT_SUCCESS;

}