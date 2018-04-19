#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>

int main()
{
	/* 输入的命令行 */
	char cmd[256];
	/* 命令行拆解成的各部分，以空指针结尾 */
	char* args[128];
	while (1)
	{
		/* 提示符 */
		printf("# ");
		fflush(stdin);
		fgets(cmd, 256, stdin);
		/* 清理结尾的换行符 */
		int i;
		int j;
		int pipenum = 0; /* 最大值是255 */
		pid_t pid;
		int pfds[256][2];
		int fd[2];
		for (i = 0; cmd[i] != '\n'; i++)
			;
		cmd[i] = '\0';
		/* 拆解命令行 */
		args[0] = cmd;
		for (i = 0; *args[i]; i++)
		{
			for (args[i + 1] = args[i] + 1; *args[i + 1]; args[i + 1]++)
				if (*args[i + 1] == ' ' || *args[i+1]== '=' || *args[i+1]=='|')
				{
					for (j = 0; *(args[i + 1]+j) == ' ' || *(args[i + 1]+j) == '|' || *(args[i+1]+j) == '='; j++)
					{
						if (*(args[i + 1]+j) == '|')
						{
							pipenum++;
						}
						*(args[i + 1]+j) = '\0';
					}
					args[i + 1] += j;
					break;
				}
		}
		args[i] = NULL;
		/* 没有输入命令 */
		if (!args[0])
			continue;
		if (strcmp(args[0], "cd") == 0) {
			if (args[1])
				chdir(args[1]);
			continue;
		}
		if (strcmp(args[0], "export") == 0) {
			if(args[1])
				setenv(args[1],args[2],1);
			continue;
		}
		for (i = 0; i <= pipenum; i++)
		{
			if (i != pipenum)
				pipe(pfds[i]);
			if ((pid = fork()) == 0) break;
		}
		if (pipenum == 1) {
			if (pid == 0) {
				if (i == 0) {
					close(pfds[0][0]);
					dup2(pfds[0][1], STDOUT_FILENO);
					close(pfds[0][1]);
					execlp(args[0], args[0], NULL);
					return 255;
				}
				else {
					close(pfds[0][1]);
					dup2(pfds[0][0], STDIN_FILENO);
					close(pfds[0][0]);
					execlp(args[1], args[1],args[2], NULL);
					return 255;
				}
			}
			else{
			close(pfds[0][0]);
			close(pfds[0][1]);
			wait(NULL);
			wait(NULL);
			continue;
			}
		}
		else if (pipenum == 2) {
			if (pid == 0) {
				if (i == 0) {
					close(pfds[0][0]);
					close(pfds[1][0]);
					close(pfds[1][1]);
					dup2(pfds[0][1], STDOUT_FILENO);
					close(pfds[0][1]);
					execlp(args[0], args[0], NULL);
					return 255;
				}
				if (i == 1) {
					close(pfds[0][1]);
					close(pfds[1][0]);
					dup2(pfds[0][0], STDIN_FILENO);
					close(pfds[0][0]);
					dup2(pfds[1][1], STDOUT_FILENO);
					close(pfds[1][1]);
					execlp(args[1], args[1], NULL);
					return 255;
				}
				else {
					close(pfds[0][0]);
					close(pfds[0][1]);
					close(pfds[1][1]);
					dup2(pfds[1][0], STDIN_FILENO);
					close(pfds[1][0]);
					execlp(args[2], args[2],args[3], NULL);
					return 255;
				}
			}
			else{
			close(pfds[0][0]);
			close(pfds[0][1]);
			close(pfds[1][0]);
			close(pfds[1][1]);
			wait(NULL);
			wait(NULL);
			wait(NULL);
			continue;
			}
		}
		else
		{
			if (pid == 0) {
				execvp(args[0], args);
				return 255;
			}
		}
		wait(NULL);
	}
}

