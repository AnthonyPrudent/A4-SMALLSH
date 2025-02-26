#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void ls() 
{
    DIR* curr_directory = opendir(".");
    struct dirent* entry;
    
    while((entry = readdir(curr_directory)) != NULL)
    {   

        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
           
            printf("%s  ", entry->d_name);
            fflush(stdout);

        }

    }

    printf("\n");
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
#define MAX_ARGS	  512


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

    printf("%s\n", curr_abs_directory);

    // CD with no arguments handling
    if(curr_command->argc == 1) {

        setenv("PWD", getenv("HOME"), 1);
    
    } else if(strcmp(curr_command->argv[1], ".") != 0) {

        // Relative path handling with dots
        if(strcmp(curr_command->argv[1], "..") == 0 || (strcmp(curr_command->argv[1], "../") == 0)) {
            
            curr_abs_directory[strlen(curr_abs_directory) - strlen(curr_rel_directory)] = '\0';

            setenv("PWD", curr_abs_directory, 1);
        
        // Absolute path handling
        } else if(opendir(curr_command->argv[1]) != NULL) {

            setenv("PWD", curr_command->argv[1], 1);
        
        // Relative path handling with directory name
        } else {

            opened_directory = opendir(getenv("PWD"));

            while((entry = readdir(opened_directory)) != NULL) {

                if(strcmp(entry->d_name, curr_command->argv[1]) == 0) {
                    
                    strcat(curr_abs_directory, "/");
                    strcat(curr_abs_directory, curr_command->argv[1]);
                    setenv("PWD", curr_abs_directory, 1);

                }

                closedir(opened_directory);

            }
        
        }

    }

    printf("%s\n", getenv("PWD"));

}

int main()
{

    struct command_line *curr_command;

    while(true)
    {

        curr_command = parse_input();
        
        if(strcmp(curr_command->argv[0], "cd") == 0) {

            cd(curr_command);

        }

    }

    return EXIT_SUCCESS;

}