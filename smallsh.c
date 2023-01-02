#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>

//Global variables
int argc;
int input_pos = 0;
int output_pos = 0;
int exit_status = 0;
int background_allowed = 1;
int background_process = 0;
pid_t background_arr[] = {0, 0, 0, 0, 0};
int process_killed = 0;

struct Command {
  int num_of_args;
  char **arguments;
  int input_pos;
  int output_pos;
} command;

//Function to change directories
void change_dir(char** args) {
    char* path;
    
    if(argc == 1) { // No arguments, changes to the directory in HOME env variable
      path = getenv("HOME");
    } else  path = args[1];
      
    if(chdir(path) == 0) {
      return;
    } else perror("Error:");
}

// Print out the exit status of the last child process
void show_status(int exit_status) {
  if (WIFEXITED(exit_status)) {
    printf("exit value %d\n", WEXITSTATUS(exit_status));
    fflush(stdout);
  } else {
    printf("terminated by signal %d\n", WTERMSIG(exit_status));
    fflush(stdout);
  }
}

//Function to execute commands
void execute_commands(int argc, char** args, struct sigaction sa_int) {
  args[argc] = NULL;
  if(argc == 0) { // A blank line was entered
    return;
  } else if (strcmp(args[0], "#") == 0 || args[0][0] == '#') { // A comment line was entered
    return;
  } else if(strcmp(args[0], "exit") == 0 && argc == 1) {//Exit the Shell program
    //kill all background processes before exiting program
    for (int i = 0; i < 5; i++) {
      if(background_arr[i] != 0) {
        kill(background_arr[i], SIGKILL);
      }
    }
    exit(0);
  } else if (strcmp(args[0], "cd") == 0) { //Change directory
            change_dir(args);
  } else if (strcmp(args[0], "status") == 0) { //Run status command
      show_status(exit_status);

  } else { 
    // Execute commands using exec functions
     pid_t pid;
     int input_fd = -2;
     int output_fd = -2;
     // Child process is created  
     pid = fork();
     int result;

     // If fork fails
     if (pid == -1) {
       perror("Unsuccessful\n");
       exit(1);
     } else if (pid == 0) { // Child process

       // If child is a foreground process then it must terminate itself on CTRL+C
       if (!background_process) {
          sa_int.sa_handler = SIG_DFL;
          sigaction(SIGINT, &sa_int, NULL);
       } 

       if (input_pos) { 
         // Redirect input 

        input_fd = open(args[input_pos], O_RDONLY);
       
        if (input_fd == -1) {
         // perror("Error");
          printf("cannot open %s for input\n", args[input_pos]);
          fflush(stdout);
          exit(1);
        }
         result = dup2(input_fd, STDIN_FILENO);
         if (result == -1) {
           perror("Could not direct input\n");
           fflush(stderr);
         }
         fcntl(input_fd, F_SETFD, FD_CLOEXEC);
         args[input_pos - 1] = 0;
         args[input_pos] = 0;
          
       } 
       if (output_pos) { 
         // Redirect output

       output_fd = open(args[output_pos], O_WRONLY | O_CREAT | O_TRUNC, 0640);
       
       if (output_fd == -1) {
         printf("cannot open %s for output\n", args[output_pos]);
         fflush(stdout);
         exit(1);
       }
         result = dup2(output_fd, STDOUT_FILENO);
         if (result == -1) {
           perror("Could not redirect output\n");
           fflush(stderr);
         }
         fcntl(output_fd, F_SETFD, FD_CLOEXEC);
         args[output_pos - 1] = 0;
         args[output_pos] = 0;
       }
       execvp(args[0], args);
       perror(args[0]);
       fflush(stderr);
       exit(1);

     } else { 
       // Parent process
        
       if (background_process) { // If background process, use WNOHANG to not wait for child
        // Store pid of child in array
         for(int i = 0; i < 5; i++) {
             if (background_arr[i] == 0) {
               background_arr[i] = pid;
               break;
             }
            }
        pid_t child_pid = waitpid(pid, &exit_status, WNOHANG);
        printf("background pid is %d \n", pid);
        fflush(stdout);
        
       } else { 
         // Process is a foreground process so wait for child to finish
           pid_t child_pid = waitpid(pid, &exit_status, 0);
          if (WIFSIGNALED(exit_status) && !process_killed) {
             printf("terminated by signal %d\n", WTERMSIG(exit_status));
             fflush(stdout);
          }
          if (process_killed) {
            process_killed = 0;
          }
       }
       
     }
     
  }
}

//Function to separate input by spaces
char** tokenize(char* input) {
  //
  char* delim = " ";
  char *token;
  char **tokens = calloc(512, sizeof(char*));
  if (tokens) {
    for (int i = 0; i < 512; i++){
      tokens[i] = (char*)calloc(2049, sizeof(char*));
    }
  }
  
  //Get rid of newline character from the input
  input[strlen(input) - 1] = 0;
  token = strtok(input, delim);
  
  int n = 0;
  while(token) {
    char *expansion = strchr(token, '$');
    char new_str[40] = {0};
    char final_str[40] = "";
    if (expansion) {

      strcpy(new_str, expansion);
      for (int i = 0; i < strlen(token); i++) {
        // separate string from $
        if (token[i] == '$') {
          token[i] = '\0';  
        }
      }
      strcpy(final_str, token);
      int expansions = strlen(new_str) / 2; // determines how many times to add pid to end of string
      pid_t pid = getpid();
      char pid_str[12] = {0};
      sprintf(pid_str, "%d", pid);
      char dollar_sign[] = {"$"};
 
      while (expansions > 0) {
        strcat(final_str, pid_str);
        expansions --;
      }
    // if there are an odd amount of $ then add one to the end
      if ((strlen(new_str) % 2) > 0) {
        strcat(final_str, dollar_sign);
      }
      strcpy(tokens[n], final_str);
    } else {
    strcpy(tokens[n], token);
    if (strcmp(token, ">") == 0) output_pos = n + 1;
    if (strcmp(token, "<") == 0) input_pos = n + 1;
    if (strcmp(token, "&") == 0) {
        tokens[n] = 0;
        if (background_allowed && (strcmp(tokens[0], "status")) != 0) background_process = 1;
    }
    }
    
    token = strtok(NULL, delim);
    n++;
  }
  argc = n;
  return tokens;
}

// Function to get input from user
char* read_input() {
  
  char* buffer;
  size_t buffsize = 256;
  size_t line;

  buffer = (char*) malloc(buffsize * sizeof(char));

  if (buffer == NULL) {
    perror("malloc() failed");
    exit(1);
  }
  line = getline(&buffer, &buffsize, stdin);
  
  return buffer;
}
// Signal handler to SIGTSTP
void handle_sigtstp(int sig) {
  if (background_allowed) {
    char *fg_message = "\nEntering foreground-only mode (& is now ignored)\n";
    write(STDOUT_FILENO, fg_message, 51);
    write(STDOUT_FILENO, ": ", 3);
    background_allowed = 0;
  } else {
      char *fg_message = "\nExiting foreground-only mode\n";
      write(STDOUT_FILENO, fg_message, 30);
      write(STDOUT_FILENO, ": ", 3);
      background_allowed = 1;

  }
}

// Signal handler for SIGCHLD
void handle_sigchld(int sig) {
  pid_t child_pid;
  int child_status = 0;
  
  while ((child_pid = waitpid(-1, &child_status, WNOHANG)) > 0) {
    char str3[12] = {0};
    char str[12] = {0};
    char str2[12] = {0};
    int pid_present = 0;
    sprintf(str, "%d", child_pid);
    exit_status = child_status;

    // Check if pid is equal to a background process
    for(int i = 0; i < 5; i++) {
      if(background_arr[i] == child_pid) {
        pid_present = 1;
        background_arr[i] = 0;
        break;
      }
    }
    
    // background process
    if (pid_present) {
    
      if (WIFEXITED(child_status)) {
      
        write(STDOUT_FILENO, "\nbackground pid ", 16);
        write(STDOUT_FILENO, str, 12);
        write(STDOUT_FILENO, " is done: ", 10);
        sprintf(str2, "%d", WEXITSTATUS(child_status));
        write(STDOUT_FILENO, "exit value ", 11);
        write(STDOUT_FILENO, str2, 12);
        write(STDOUT_FILENO, "\n", 1);
        write(STDOUT_FILENO, ": ", 2);
        
  }   else {
        process_killed = 1; // Used to avoid duplicate prints when a foreground process is terminated
        write(STDOUT_FILENO, "background pid ", 15);
        write(STDOUT_FILENO, str, 12);
        write(STDOUT_FILENO, " is done: ", 10);
        sprintf(str2, "%d", WTERMSIG(child_status));
        write(STDOUT_FILENO, "terminated by signal ", 21);
        write(STDOUT_FILENO, str2, 12);
        write(STDOUT_FILENO, "\n", 1);
        
  }

    } 
  }
}

// Main function that uses an infinite for-loop to simulate a shell
int main(void)
{

  // Handle CTRL+Z SIGTSTP
  struct sigaction sa_stop;
  sa_stop.sa_handler = &handle_sigtstp;
  sigfillset(&sa_stop.sa_mask);
  sa_stop.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &sa_stop, NULL);

  //Handle CTRL+C SIGINT
  struct sigaction sa_int;
  sa_int.sa_handler = SIG_IGN;
  sigfillset(&sa_int.sa_mask);
  sa_int.sa_flags = SA_RESTART;
  sigaction(SIGINT, &sa_int, NULL);

  // Watch for terminated children
  struct sigaction sa_child;
  sa_child.sa_handler = &handle_sigchld;
  sa_child.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa_child, NULL);

  while (true) {
    printf(": ");
    fflush(stdin);
    char* input = read_input();

    //Separate input by spaces
    char** tokens = tokenize(input);

    //Execute commands including exit, cd, and status
   execute_commands(argc, tokens, sa_int);

    free(input);
    for (int i =0; i < 512; i++) {
      free(tokens[i]);
    }
    free(tokens);
    input_pos = 0;
    output_pos = 0;
    background_process = 0;
  }
  

  return 0;
}
