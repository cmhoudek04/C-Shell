#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Define the default size of the memory block for an input command
#define SHELL_RL_BUFSIZE 1024

char *shell_read_line(void) {
    int bufsize = SHELL_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize); // Manually allocate a block of memory that stores bufsize number of chars
    int c; // This needs to be an integer because EOF is an integer. If we want to check for EOF later, this variable must also be an int.

    // If memory block fails to allocate
    if (!buffer) {
        fprintf(stderr, "shell: allocation error\n"); // Write error message to the standard error message stream
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Read a character
        c = getchar();

        // If we hit EOF (end of file), replace it with a null character and return
        if (c == EOF || c == '\n') {
            buffer[position] = '\0'; // set buffer position to a null character
            return buffer;
        }
        else {
            buffer[position] = c;
        }
        position++;

        // If we have exceded the buffer, reallocate.
        if (position >= bufsize) {
            bufsize += SHELL_RL_BUFSIZE; // Double buffer size
            buffer = realloc(buffer, bufsize); // Reallocate memory (this does not drop the data from the original buffer, just reallocates new memory to store it)

            // Check if allocation fails like before
            if (!buffer) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

// This function is written assuming that we won't allow quoting or backslash escaping in the shell
#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM "\t\r\n\a"
char **shell_split_line(char *line) {
    int bufsize = SHELL_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*)); // Pointer to a block of pointers to token values
    char *token; // Pointer to token values

    // If tokens fails to allocate
    if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Split the line into tokens deliminated by SHELL_TOK_DELIM
    token = strtok(line, SHELL_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        // If the position exceeds the size of the buffer
        if (position >= bufsize) {
            bufsize += SHELL_TOK_BUFSIZE; // Double the buffer size
            tokens = realloc(tokens, bufsize * sizeof(char*));

            // Check for failure to allocate
            if (!tokens) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, SHELL_TOK_DELIM); // NULL here tells the function to continue parsing from where the last call left off
    }
    tokens[position] = NULL;
    return tokens;
}

int shell_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork(); // Unfortunately I cannot run this as I am on a windows machine. I will have to test this on a linux VM or build later
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("shell");
        }
        exit(EXIT_SUCCESS);
    }
    else if (pid < 0) {
        // Error forking
        perror("shell");
    }
    else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/*
 * Function declarations for built in shell commands
 */
int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);

/*
 * List of built in shell commands, followed by their corresponding functions
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

/*
 * Adding this comment for my own understanding:
 *
 * This function declaration is an array of function pointers that take an array of strings and return an int.
 */
int (*builtin_func[]) (char **) = {
    &shell_cd,
    &shell_help,
    &shell_exit
};

int shell_num_bultins() {
    return sizeof(builtin_func) / sizeof(char *);
}

/*
 * Built in function declarations
 */
int shell_cd(char **args) {
    // If there is no path argument for cd, return error
    if (args[1] == NULL) {
        fprintf(stderr, "shell: expected argument to \"cd\"\n");
    }
    else {
        // If attempt to change directory fails
        if (chdir(args[1]) != 0) {
            perror("shell");
        }
    }
    return 1; // Return success
}

int shell_help(char **args) {
    int i;
    printf("Conner Houdek's basic C shell. Designed after Stephen Brennan's LSH.\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following commands are built in:\n");

    // Loop through the list of built in commands and display them with the help command
    for (i = 0; i < shell_num_bultins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int shell_exit(char **args) {
    return 0;
}

int shell_execute(char **args) {
    int i;

    // If an empty command is given
    if (args[0] == NULL) {
        return 1;
    }

    // Check if the command given in args matches any of the built in functions and if it does, run it
    for (i = 0; i < shell_num_bultins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    // If the command in args doesn't match a built in command, run shell_launch() to launch the process.
    return shell_launch(args);
}

void shell_loop(void) {
    char *line; // This is the line that is entered into the shell by the user
    char **args; // This is a segmented string that stores the arguments from the input line
    int status; // Stores the status of the shell

    do {
        printf("> "); // Print a prompt to the shell
        line = shell_read_line(); // Read the input line
        args = shell_split_line(line); // Segment the line into arguments
        status = shell_execute(args); // Check the status of the shell

        free(line); // Free memory allocated to line
        free(args); // Free memory allocated to args
    } while (status); // Execute the shell
}

int main(int argc, char **argv) {
    // Load config files, if any

    // Run command loop
    shell_loop();

    // Perform any shutdown/cleanup

    return EXIT_SUCCESS;
}
