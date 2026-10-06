#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

static void
execute_command_line(const struct command_line *line)
{
	/* REPLACE THIS CODE WITH ACTUAL COMMAND EXECUTION */

	assert(line != NULL);
	printf("================================\n");
	printf("Command line:\n");
	printf("Is background: %d\n", (int)line->is_background);
	printf("Output: ");
	if (line->out_type == OUTPUT_TYPE_STDOUT)
	{
		printf("stdout\n");
	}
	else if (line->out_type == OUTPUT_TYPE_FILE_NEW)
	{
		printf("new file - \"%s\"\n", line->out_file);
	}
	else if (line->out_type == OUTPUT_TYPE_FILE_APPEND)
	{
		printf("append file - \"%s\"\n", line->out_file);
	}
	else
	{
		assert(false);
	}
	printf("Expressions:\n");
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
			if (!strcmp(e->cmd.exe, "cd"))
			{
				if (args[1] == NULL)
				{
					chdir(getenv("HOME"));
				}
				else
				{
					if (chdir(args[1]))
					{
						perror(e->cmd.exe);
					}
				}
				free(args);
				e = e->next;
				continue;
			}
			if (!strcmp(e->cmd.exe, "exit"))
			{
				int exit_code = 0;
    				if (args[1] != NULL)
				{
    			    		exit_code = atoi(args[1]);
    				}
    				free(args);
    				exit(exit_code);
			}
			bool has_pipe = (e->next && e->next->type == EXPR_TYPE_PIPE);
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
		else if (e->type == EXPR_TYPE_PIPE)
		{
			printf("\tPIPE\n");
		}
		else if (e->type == EXPR_TYPE_AND)
		{
			printf("\tAND\n");
		}
		else if (e->type == EXPR_TYPE_OR)
		{
			printf("\tOR\n");
		}
		else
		{
			assert(false);
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
				printf("Error: %d\n", (int)err);
				continue;
			}
			execute_command_line(line);
			command_line_delete(line);
		}
	}
	parser_delete(p);

	// int pipefd[2];
	// pid_t pid1, pid2;

	// pipe(pipefd);

	// pid1 = fork();

	// if (pid1 == 0)
	// {
	// 	dup2(pipefd[1], STDOUT_FILENO);
	// 	close(pipefd[0]);
	// 	close(pipefd[1]);
	// 	char* args[] = {"ls", NULL};
	// 	execvp("ls", args);
	// }

	// pid2 = fork();

	// if (pid2 == 0)
	// {
	// 	dup2(pipefd[0], STDIN_FILENO);
	// 	close(pipefd[0]);
	// 	close(pipefd[1]);
	// 	char* args[] = {"grep", ".c", NULL};
	// 	execvp("grep", args);
	// }

	// close(pipefd[0]);
	// close(pipefd[1]);

	// waitpid(pid1, NULL, 0);
    // waitpid(pid2, NULL, 0);
    
	return 0;
}
