#include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int prepare(void)
{
	return 0;
}

int process_arglist(int count, char **arglist)
{
	int stat_loc;
	int child_pid,child1_pid,child2_pid;
	struct sigaction sa_dfl,sa_ign;
	sa_dfl.sa_handler = SIG_DFL;
	sa_dfl.sa_flags = SA_RESTART;
	sa_ign.sa_handler = SIG_IGN;
	sigaction(SIGINT,&sa_ign,NULL);
	for(int i = 0;i < count;++i)
	{
		if (strcmp(arglist[i], "|") == 0)
		{
			arglist[i] = '\0';
			int pipefd[2];
			if (pipe(pipefd) < 0)
			{
				fprintf(stderr,"pipe can't be initialized\n");
				return 0;
			}
			child1_pid = fork();
			if (child1_pid < 0)
			{
				fprintf(stderr, "%s","Fork failed");
				return 0;
			}
			if (child1_pid == 0)
			{
				sigaction(SIGINT,&sa_dfl,NULL);
				close(pipefd[0]);
				dup2(pipefd[1],STDOUT_FILENO);
				close(pipefd[1]);
				 if (execvp(arglist[0], arglist) < 0)
				{
					fprintf(stderr, "%s",arglist[0]);
				}
			}
			else
			{
				child2_pid = fork();
				if (child2_pid < 0)
				{
					fprintf(stderr, "%s","Fork failed");
					return 0;
				}
				if (child2_pid == 0)
				{
					sigaction(SIGINT,&sa_dfl,NULL);
					close(pipefd[1]);
					dup2(pipefd[0],STDIN_FILENO);
					close(pipefd[0]);
			 		if (execvp(arglist[i + 1],&arglist[i + 1]) < 0)
					{
						fprintf(stderr, "%s",arglist[0]);
					}
				}
				else
				{
					close(pipefd[0]);
					close(pipefd[1]);
					waitpid(child1_pid, &stat_loc, WUNTRACED);
					waitpid(child2_pid, &stat_loc, WUNTRACED);
				}
			}
			return 1;
		}
	}
	if (strcmp(arglist[count -1], "&") == 0)
	{
		arglist[count - 1] ='\0';
		int child_pid = fork();
		if (child_pid < 0)
		{
			fprintf(stderr, "%s","Fork failed");
			return 0;
		}
		if (child_pid == 0)
		{
			sigaction(SIGINT,&sa_ign,NULL);
			 if (execvp(arglist[0], arglist) < 0)
			{
				fprintf(stderr, "%s",arglist[0]);
			}
		}
	}
	
	else
	{
		child_pid = fork();
		if (child_pid < 0)
		{
			fprintf(stderr, "%s","Fork failed");
			return 0;
		}
		if (child_pid == 0)
		{
			sigaction(SIGINT,&sa_dfl,NULL);
			if (execvp(arglist[0], arglist) < 0)
			{
				fprintf(stderr, "%s",arglist[0]);
				exit(1);
			}
		}
		else
		{
			waitpid(child_pid, &stat_loc, WUNTRACED);
		}
	}
	return 1;
}


int finalize(void)
{
	return 0;
}