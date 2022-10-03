#include <linux/limits.h>
#include <unistd.h>
#include <termios.h>
#include <wait.h>
#include <signal.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char *readcmd(pid_t pid);
static void killprompt(pid_t pid);

char *
readcmd(pid_t pid)
{
	char pathbuf[PATH_MAX];
	size_t i, cap, size;
	FILE *file;
	char *cmd;

	snprintf(pathbuf, PATH_MAX, "/proc/%u/cmdline", pid);
	file = fopen(pathbuf, "r");
	if (!file) return NULL;

	cap = 4096;
	cmd = malloc(cap + 1);
	if (!cmd) err(1, "malloc");

	if (!(size = fread(cmd, 1, cap, file))) {
		free(cmd);
		fclose(file);
		return NULL;
	}

	for (i = 0; i < size; i++)
		if (!cmd[i]) cmd[i] = ' ';
	cmd[size] = '\0';

	fclose(file);

	return cmd;
}

void
killprompt(pid_t pid)
{
	struct termios old, new;
	char c;

	tcgetattr(0, &old);
	tcgetattr(0, &new);
	new.c_lflag &= ~ECHO;
	new.c_lflag &= ~ICANON;
	new.c_lflag &= ~ISIG;
	tcsetattr(0, TCSANOW, &new);

	c = getchar();
	if (c == 'y') {
		kill(pid, SIGTERM);
		waitpid(pid, NULL, 0);
	}

	tcsetattr(0, TCSANOW, &old);

	if (c == 3) exit(0);
}

int
main(int argc, const char **argv)
{
	pid_t pids[256];
	int i, pidcnt;
	char *end;
	char *cmd;

	setvbuf(stdin, NULL, _IONBF, 0);

	pidcnt = 0;
	for (i = 1; i < argc; i++) {
		if (!*argv[i]) continue;
		pids[pidcnt++] = strtoul(argv[i], &end, 10);
		if (end && *end) errx(1, "Invalid pid: %s", argv[i]);
	}

	for (i = 0; i < pidcnt; i++) {
		if (!kill(pids[i], 0)) {
			cmd = readcmd(pids[i]);
			printf("%u: %s", pids[i], cmd ? cmd : "???");
			free(cmd);
			killprompt(pids[i]);
			printf("\n");
			pids[i] = -1;
			break;
		} else {
			fprintf(stderr, "%i: Not running\n", pids[i]);
		}
	}
}
