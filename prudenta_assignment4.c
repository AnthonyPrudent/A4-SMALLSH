/*
* Code adapted from:
* Title: sample_parser.c
* Author: Oregon State University 
* Date 2/25/2025
* Availability: https://canvas.oregonstate.edu/courses/1987883/files/109834045?wrap=1
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
 
void ls() 
{


}
 
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
 
int main()
{
    struct command_line *curr_command;

    while(true)
    {
        curr_command = parse_input();

        for(int i=0; i < curr_command->argc; i++)
        {
            if(strcmp(curr_command->argv[i], "ls") == 0) {
                ls();
            }
        }

    }
    return EXIT_SUCCESS;
}