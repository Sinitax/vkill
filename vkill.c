#include <linux/limits.h>
#include <strings.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <wait.h>
#include <ctype.h>
#include <signal.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *strcasestr(const char *haystack, const char *needle);

static char *readcmd(pid_t pid);
static void killprompt(pid_t pid);

static const char usage[] = "vkill [-h] [-SIG] CMD";
static int signum = SIGTERM;

const char *
strcasestr(const char *haystack, const char *needle)
{
	size_t needle_len;

	needle_len = strlen(needle);
	while (*haystack) {
		if (!strncasecmp(haystack, needle, needle_len))
			return haystack;
		haystack++;
	}

	return NULL;
}

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
		kill(pid, signum);
		waitpid(pid, NULL, 0);
	}

	tcsetattr(0, TCSANOW, &old);

	if (c == 3) exit(0);
}

int
main(int argc, const char **argv)
{
	pid_t pid, cpid;
	struct dirent *ent;
	const char **arg;
	const char *query;
	char *end, *cmd;
	DIR *dir;
	int i;

	query = NULL;
	for (arg = &argv[1]; *arg; arg++) {
		if (!strcmp(*arg, "-h")) {
			printf("Usage: %s\n", usage);
			return 0;
		} else if (**arg == '-') {
			signum = strtoul(*arg+1, &end, 10);
			if (end && *end) errx(1, "Invalid signum");
		} else if (!query) {
			query = *arg;
		} else {
			fprintf(stderr, "Usage: %s\n", usage);
			return 1;
		}
	}

	if (!query) {
		fprintf(stderr, "Usage: %s\n", usage);
		return 1;
	}

	cpid = getpid();

	dir = opendir("/proc");
	if (!dir) err(1, "opendir");

	while ((ent = readdir(dir))) {
		pid = strtoul(ent->d_name, &end, 10);
		if (end && *end) continue;
		if (pid == cpid) continue;

		cmd = readcmd(pid);
		for (i = 1; i < argc; i++) {
			if (cmd && strcasestr(cmd, argv[i])) {
				printf("%u: %s", pid, cmd ? cmd : "???");
				killprompt(pid);
				printf("\n");
				break;
			}
		}
		free(cmd);
	}

	closedir(dir);
}
