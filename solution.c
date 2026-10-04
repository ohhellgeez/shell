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
	while (e != NULL)
	{
		if (e->type == EXPR_TYPE_COMMAND)
		{
    		char **args = calloc(e->cmd.arg_count + 2, sizeof(char*));
    
    		args[0] = e->cmd.exe;
    
    		for (uint32_t i = 0; i < e->cmd.arg_count; ++i) {
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
			pid_t pid = fork();
			if (pid == -1) 
			{
				perror("fork failed");
    			free(args);
			}
			else if (pid == 0)
			{
				execvp(e->cmd.exe, args);
				perror(e->cmd.exe);
				free(args);
				exit(1);
			}
			else
			{
				free(args);
				int status;
				waitpid(pid, &status, 0);
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
