#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

static void
execute_command_line(const struct command_line *line)
{
     if (line == NULL || line->head == NULL)
          return;

     const struct expr *e = line->head;
     int in_fd = 0;

     while (e != NULL)
     {
          if (e->type == EXPR_TYPE_COMMAND)
          {
               char **args = calloc(e->cmd.arg_count + 2, sizeof(char*));
               args[0] = e->cmd.exe;
               for (uint32_t i = 0; i < e->cmd.arg_count; ++i)
               {
                    args[i + 1] = e->cmd.args[i];
               }
               args[e->cmd.arg_count + 1] = NULL;

               bool has_pipe = (e->next && e->next->type == EXPR_TYPE_PIPE);
               bool is_in_pipe = (in_fd != 0 || has_pipe);

               if (!strcmp(e->cmd.exe, "cd"))
               {
                    if (!is_in_pipe)
                    {
                         if (args[1] == NULL) {
                              chdir(getenv("HOME"));
                         } else {
                              if (chdir(args[1])) perror(e->cmd.exe);
                         }
                         free(args);
                         e = e->next;
                         continue;
                    }
               }
               else if (!strcmp(e->cmd.exe, "exit"))
               {
                    if (!is_in_pipe)
                    {
                         int exit_code = 0;
                         if (args[1] != NULL) exit_code = atoi(args[1]);
                         free(args);
                         exit(exit_code);
                    }
               }

               int fd[2];
               if (has_pipe)
               {
                    if (pipe(fd) == -1)
                    {
                         perror("pipe");
                         free(args);
                         break;
                    }
               }

               pid_t pid = fork();
               if (pid == -1) 
               {
                    perror("fork failed");
                    free(args);
                    break;
               }
               else if (pid == 0)
               {
                    if (in_fd != 0)
                    {
                         dup2(in_fd, STDIN_FILENO);
                         close(in_fd);
                    }
                    
                    if (has_pipe)
                    {
                         dup2(fd[1], STDOUT_FILENO);
                         close(fd[1]);
                         close(fd[0]);
                    }
                    else
                    {
                         if (line->out_type == OUTPUT_TYPE_FILE_NEW || line->out_type == OUTPUT_TYPE_FILE_APPEND)
                         {
                              int flags = O_WRONLY | O_CREAT;
                              flags |= (line->out_type == OUTPUT_TYPE_FILE_NEW) ? O_TRUNC : O_APPEND;
                              
                              int out_fd = open(line->out_file, flags, 0666);
                              if (out_fd == -1)
                              {
                                   perror(line->out_file);
                                   exit(1);
                              }
                              dup2(out_fd, STDOUT_FILENO);
                              close(out_fd);
                         }
                    }

                    if (!strcmp(e->cmd.exe, "cd")) {
                         int ret = (args[1] == NULL) ? chdir(getenv("HOME")) : chdir(args[1]);
                         if (ret) perror(e->cmd.exe);
                         exit(ret == 0 ? 0 : 1);
                    }
                    if (!strcmp(e->cmd.exe, "exit")) {
                         int exit_code = (args[1] != NULL) ? atoi(args[1]) : 0;
                         exit(exit_code);
                    }

                    execvp(e->cmd.exe, args);
                    perror(e->cmd.exe);
                    free(args);
                    exit(1);
               }
               else
               {    
                    free(args);
                    
                    if (in_fd != 0)
                    {
                         close(in_fd);
                    }
                    
                    if (has_pipe)
                    {
                         close(fd[1]); 
                         in_fd = fd[0]; 
                         e = e->next;
                    }
                    else
                    {
                         int status;
                         waitpid(pid, &status, 0);
                    }
               }
          }
          e = e->next;
     }
}

int
main(void)
{
     const size_t buf_size = 1024;
     char buf[buf_size];
     int rc;
     struct parser *p = parser_new();
     
     while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0)
     {
          parser_feed(p, buf, rc);
          struct command_line *line = NULL;
          while (true)
          {
               enum parser_error err = parser_pop_next(p, &line);
               if (err == PARSER_ERR_NONE && line == NULL)
                    break;
               if (err != PARSER_ERR_NONE)
               {
                    continue;
               }
               
               execute_command_line(line);
               command_line_delete(line);
          }
     }
     
     parser_delete(p);
     return 0;
}