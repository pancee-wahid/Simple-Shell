#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>

#define MAX_LINE_LENGTH 400
#define MAX_WORD_LENGTH 100
#define MAX_WORDS_NO 10
#define MAX_VARIABLES_NO 20

// for export
int available_space = MAX_VARIABLES_NO;
char *var_name[MAX_VARIABLES_NO] = {};
char *var_value[MAX_VARIABLES_NO] = {};

// for running in backgroud or foreground
int background = 0;

/* Functions Declarations */
void on_child_exit();
void setup_environment();
void shell();
int parse_execute(char cmd[], char *cmd_args[]);
void change_directory(char arg[]);
void export(char arg[]);
void echo(char arg[]);
void execute_command(char cmd_type[], char *cmd_args[]);
int evaluate_args(char arg[], char evaluated[]);

/* parent_main function */
int main()
{
   /* initialization */
   for (int i = 0; i < MAX_VARIABLES_NO; i++)
   {
      var_name[i] = (char *)malloc(MAX_WORD_LENGTH * sizeof(char));
      var_value[i] = (char *)malloc(MAX_WORD_LENGTH * sizeof(char));
   }

   /* register_child_signal(on_child_exit()) */
   signal(SIGCHLD, on_child_exit);

   /* environment setup */
   if (chdir(getenv("HOME")) != 0)
      printf("ERROR: Could't change directory to home directory\n");

   /* shell() */
   shell();

   return 0;
}

/* Interrupt handler to remove zombie process */
void on_child_exit()
{
   /* reap zombie process */
   int wstat;
   pid_t pid;

   while (1)
   {
      pid = wait3(&wstat, WNOHANG, (struct rusage *)NULL);
      if (pid == 0 || pid == -1)
         break;
   }
   // printf("Child terminated");
   /* write to log file  */
   // open the file for appending (or creation if it doesn't exist)
   FILE *log_file;
   log_file = fopen("log.txt", "a");

   // print error msg in case of error while creating the file
   if (log_file == NULL)
   {
      printf("Error in creating log file!");
   }

   // write the termination msg to the log file and close it
   // char msg[] = "Child process was terminated\n";
   // fwrite(msg , 1 , sizeof(msg) , log_file);
   fprintf(log_file, "Child process was terminated\n");
   fclose(log_file); // close the log file
   return;
}

void shell()
{
   int is_exit = 0;
   do
   {
      char cmd[MAX_LINE_LENGTH];
      char *cmd_args[MAX_WORDS_NO];
      for (int i = 0; i < MAX_WORDS_NO; i++)
      { // try to delete
         cmd_args[i] = (char *)malloc(MAX_WORD_LENGTH * sizeof(char));
      }
      fgets(cmd, MAX_LINE_LENGTH, stdin);
      is_exit = parse_execute(cmd, cmd_args);
   } while (!is_exit);
   exit(0);
}

int parse_execute(char cmd[], char *cmd_args[])
{
   char *context = NULL;
   int i = 0;
   for (i = 0; cmd[i] != '\n'; i++)
   {
      ;
   }
   cmd[i] = '\0';

   // get command type
   char *cmd_type = strtok_r(cmd, " ", &context);

   if (cmd_type == NULL || strcmp(cmd_type, "") == 0)
   { // no command
      printf("No Command is entered!\n");
      return 0;
   }
   else if (strcmp(cmd_type, "exit") == 0)
   { // exit command
      return 1;
   }

   // built-in commands
   if (strcmp(cmd_type, "cd") == 0)
   {
      change_directory(context);
   }
   else if (strcmp(cmd_type, "echo") == 0)
   {
      echo(context);
   }
   else if (strcmp(cmd_type, "export") == 0)
   {
      export(context);
   }
   else
   {
      // non-built-in commands
      cmd_args[0] = cmd_type;
      char *context_args = NULL;
      char *token = strtok_r(context, " ", &context_args);
      int k = 1;
      while (token != NULL)
      {
         cmd_args[k++] = token;
         token = strtok_r(NULL, " ", &context_args);
      }
      cmd_args[k] = NULL;
      execute_command(cmd_type, cmd_args);
   }
   return 0;
}

void change_directory(char arg[])
{
   char s[MAX_WORD_LENGTH];
   /* cd(Current_Working_Directory) */
   if (arg == NULL || strcmp(arg, "") == 0 || strcmp(arg, "~") == 0)
   { // cd to home
      if (chdir(getenv("HOME")) != 0)
         printf("ERROR: Could't change directory to home directory\n");
      printf("Current Directory: %s\n", getcwd(s, MAX_WORD_LENGTH));
   }
   else if (strcmp(arg, "..") == 0)
   { // cd to parent dir
      if (chdir("..") != 0)
         printf("ERROR: Could't change directory to parent directory\n");
      printf("Current Directory: %s\n", getcwd(s, MAX_WORD_LENGTH));
   }
   else
   { // relative or absolute path
      char dir[250];
      char edited_arg[MAX_WORD_LENGTH + 1] = "/";
      if (arg[0] != '/')
         strcpy(edited_arg, strcat(edited_arg, arg));
      else
         strcpy(edited_arg, arg);
      if (strncmp(edited_arg, "/usr/home", 10) == 0) 
         strcpy(dir, edited_arg); // absolute path
      else
         strcpy(dir, strcat(getcwd(s, MAX_WORD_LENGTH), edited_arg)); // relative path
      if (chdir(dir) != 0)
         printf("ERROR: Could't change directory to %s\n", arg);
      printf("Current Directory: %s\n", getcwd(s, MAX_WORD_LENGTH));
   }
   return;
}

void export(char arg[])
{
   char *context = NULL;
   char *context_2 = NULL;
   char *context_3 = NULL;

   char *token = strtok_r(arg, "=", &context); // context has what's after the "=" and token has what's before it
   // remove spaces before the equal if exist
   token = strtok_r(token, " ", &context_2); // token has the LHS without leading & trailing spaces if exist
   int index = MAX_VARIABLES_NO - available_space;
   if (token != NULL)
   {
      strcpy(var_name[index], token);
      // if (context[0] == ' ') {  // remove spaces after the equal if exist
      //    token = strtok_r(context, " ", &context_3);
      //    token = strtok_r(token, "\"", &context_3); // remove double quotes if exist
      // } else {
      token = strtok_r(context, "\"", &context_3);
      // }
      if (token == NULL)
      { // no value entered
         available_space++;
         printf("Invalid argument!\n");
      }
      else
      {
         strcpy(var_value[index], token);
         available_space--;
      }
   }
   return;
}

void echo(char arg[])
{
   char to_print[MAX_LINE_LENGTH];
   memset(to_print, '\0', MAX_LINE_LENGTH * sizeof(char));

   int stat = evaluate_args(arg, to_print);
   if (stat == 1)
      printf("%s\n", to_print);
   else if (stat == 0)
      printf("Undefined variable!\n");
   else // -1
      printf("No arguments passed!");
   return;
}

int evaluate_args(char arg[], char evaluated[])
{ // returns 0 in case of undefined var, -1 in case of null arg, 1 otherwise
   int found = 0;
   char *token = arg;

   if (arg == NULL)
   {
      return -1;
   }
   else if (arg[0] == '\"')
   { // double quoted
      char *context = NULL;
      token = strtok_r(arg, "\"", &context);
   }

   for (int i = 0; token[i] != '\0'; i++)
   {
      if (token[i] == '$')
      {
         i++;
         char v[MAX_WORD_LENGTH];
         memset(v, '\0', MAX_WORD_LENGTH * sizeof(char));

         int j = 0;
         while (token[i] != '\0' && token[i] != ' ')
         {
            v[j++] = token[i++];
         }
         i--; // as i will be incremented again by the main loop

         for (int k = 0; k < MAX_VARIABLES_NO - available_space; k++)
         {
            if (strcmp(var_name[k], v) == 0)
            { // found the variable
               strcat(evaluated, var_value[k]);
               found = 1;
               break;
            }
         }
         if (found)
            found = 0;
         else
            return 0;
      }
      else
         strncat(evaluated, &token[i], 1);
   }
   return 1;
}

void execute_command(char cmd_type[], char *cmd_args[])
{
   if (cmd_args[1] != NULL && strcmp(cmd_args[1], "&") == 0)
   {
      background = 1;
   }
   int cstatus;
   int cpid = fork();

   if (cpid == 0)
   { // in child
      // evaluate arguments
      char *evaluated_args[MAX_WORDS_NO];
      int evaluation_state = 0;
      strcpy(evaluated_args[0], cmd_args[0]);
      int i, k;
      for (i = (background == 1) ? 2 : 1, k = 1; cmd_args[i] != NULL; i++, k++)
      {
         evaluated_args[k] = (char *)malloc(MAX_WORD_LENGTH * sizeof(char));
         evaluation_state = evaluate_args(cmd_args[i], evaluated_args[k]);
         if (evaluation_state == 0)
         {
            printf("Undefined variable!\n");
            return;
         }
         else if (evaluation_state == -1)
         {
            printf("No arguments passed!");
            return;
         }
         if (evaluated_args[k] != NULL)
         {
            char *con = NULL;
            char *to_split = (char *)malloc(MAX_WORD_LENGTH * sizeof(char));
            strcpy(to_split, evaluated_args[k]);
            char *tok = strtok_r(to_split, " ", &con);
            while (tok != NULL)
            {
               evaluated_args[k] = (char *)malloc(MAX_WORD_LENGTH * sizeof(char));
               strcpy(evaluated_args[k], tok);
               tok = strtok_r(NULL, " ", &con);
               k++;
            }
            k--;
         }
      }
      evaluated_args[k] = NULL;

      // execute command
      int status = execvp(cmd_type, evaluated_args);
      if (status < 0)
      { // error occured within execvp()
         // terminate the childe process
         printf("ERROR: error occured on executing %s\n", cmd_type);
         exit(1);
      }
      else
      {
         exit(0);
      }
   }
   else if (cpid > 0 && !background)
   { // parent and foreground run mode
      do
      {
         cpid = waitpid(cpid, &cstatus, WNOHANG);
      } while (cpid == 0);
   }
   background = 0;
   return;
}