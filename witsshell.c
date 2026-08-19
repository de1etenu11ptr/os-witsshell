#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFFERSIZE 1024
#define MAXTOKENS 64
#define MAXCMDS 64

#ifdef DEBUG
#define LOG_ERR(fmt, ...)                                             \
	fprintf(stderr, "%s:%d (%s) - " fmt "\n", __FILE__, __LINE__, \
		__func__ __VA_OPT__(, ) __VA_ARGS__)
#else
#define LOG_ERR(fmt, ...)                                                   \
	do {                                                                \
		char error_message[30] = "An error has occurred\n";         \
		write(STDERR_FILENO, error_message, strlen(error_message)); \
	} while (false)

#endif

#define IS_QUOTE(chr) chr == '"' || chr == '\'' || chr == '`'
#define IS_PARALLEL(chr) chr == '&'
#define IS_REDIRECTION(chr) chr == '>'

static char **paths = NULL;
static int no_paths = 0;

void free_paths()
{
	for (int i = 0; i < no_paths; i++)
		if (paths[i] != NULL) {
			free(paths[i]);
			paths[i] = NULL;
		}
	if (paths != NULL) {
		free(paths);
		paths = NULL;
	}
	no_paths = 0;
}

struct built_in_command {
	const char *command;
	const int no_args;
	void (*func)(char **, int);
};

void program_exit(char **tokens, int no_tokens)
{
	exit(1);
}

void change_directory(char **tokens, int no_tokens)
{
	if (chdir(tokens[1]) == -1)
		LOG_ERR("%s", strerror(errno));
}

void change_path_list(char **tokens, int no_tokens)
{
	free_paths();
	no_paths = no_tokens - 1;
	if (no_tokens == 0)
		return;
	paths = malloc(sizeof(char *) * no_paths);
	for (int i = 1; i <= no_paths; i++) {
		int len = strlen(tokens[i]);
		paths[i - 1] = malloc(sizeof(char) * (len + 1));
		memcpy(paths[i - 1], tokens[i], len);
		paths[i - 1][len] = '\0';
	}
}

void print_path_list(char **tokens, int no_tokens)
{
	for (int i = 0; i < no_paths; i++) {
		printf("%s\n", paths[i]);
	}
}

static const struct built_in_command built_in_cmds[] = {
	{ "exit", 0, program_exit },
	{ "cd", 1, change_directory },
	{ "path", -1, change_path_list },
#ifdef DEBUG
	{ "print_path", 0, print_path_list }
};
static const int no_built_in_cmds = 4;
#else
};
static const int no_built_in_cmds = 3;
#endif

int built_in(char **tokens, int no_tokens)
{
	for (int i = 0; i < no_built_in_cmds; i++) {
		if (built_in_cmds[i].no_args != -1 &&
		    no_tokens - 1 != built_in_cmds[i].no_args)
			continue;
		if (strcmp(built_in_cmds[i].command, tokens[0]) == 0) {
			built_in_cmds[i].func(tokens, no_tokens);
			return 1;
		}
	}
	return 0;
}

void print_tokens(char **tokens, int no_tokens)
{
	for (int i = 0; i < no_tokens; i++) {
		printf("arg %d: %s\n", i, tokens[i]);
	}
	fflush(stdout);
}

void free_tokens(char **tokens, int no_tokens)
{
	for (int i = 0; i < no_tokens; i++)
		if (tokens[i] != NULL) {
			free(tokens[i]);
			tokens[i] = NULL;
		}
}

char *create_token(char *start, char *end)
{
	int len = end - start;
	char *token = malloc(sizeof(char) * (len + 1));
	memcpy(token, start, len);
	token[len] = '\0';
	return token;
}

int tokenize(char **tokens, int max_tokens, char *cmd, int cmd_len)
{
	char *token = cmd, quote;
	int i = 0, is_quoted = 0, no_tokens = 0;

	while (i <= cmd_len && no_tokens < max_tokens - 1) {
		if (is_quoted) {
			if (cmd[i] == quote && cmd[i - 1] != '\\') {
				is_quoted = 0;
			}
		} else if (IS_QUOTE(cmd[i])) {
			is_quoted = 1;
			quote = cmd[i];
		} else if (IS_PARALLEL(cmd[i]) || IS_REDIRECTION(cmd[i])) {
			if (cmd[i - 1] == ' ' || IS_PARALLEL(cmd[i - 1]) ||
			    IS_REDIRECTION(cmd[i - 1]))
				tokens[no_tokens++] =
					create_token(token, token + 1);
			else {
				// create token of previous "word"
				tokens[no_tokens++] =
					create_token(token, cmd + i);
				token = cmd + i;
				// create token of special symbol
				tokens[no_tokens++] =
					create_token(token, token + 1);
			}
			token = cmd + i + 1;
		} else if (cmd[i] == ' ' || cmd[i] == '\0') {
			if (strncmp(token, "", 1) != 0 &&
			    strncmp(token, " ", 1) != 0)
				tokens[no_tokens++] =
					create_token(token, cmd + i);
			token = cmd + i + 1;
		}
		i++;
	}
	if (is_quoted) {
		LOG_ERR("Failed to tokenize due to the unmatched quote \"%c\"",
			quote);
		return 0;
	}

	tokens[no_tokens] = NULL;
	return no_tokens;
}

void token_preprocessor(char **tokens, int no_tokens)
{
	for (int i = 0; i < no_tokens; i++) {
		char *token = tokens[i];
		int read = 0, write = 0;

		while (token[read]) {
			if (IS_QUOTE(token[read])) {
				read++;
				continue;
			}
			token[write++] = token[read++];
		}
		token[write] = '\0';
	}
}

int get_input_string(char *buffer, int buffersize)
{
	int i = 0, is_quoted = 0;
	char c, quote;

	while (i < buffersize - 1) {
		if ((c = fgetc(stdin)) == '\n' && !is_quoted) {
			break;
		} else if (is_quoted && c == quote && buffer[i - 1] != '\\') {
			is_quoted = 0;
		} else if (IS_QUOTE(c)) {
			is_quoted = 1;
			quote = c;
		} else if (c == '\n') {
			buffer[i++] = ' ';
			printf(">");
			continue;
		}
		buffer[i++] = c;
	}
	buffer[i] = '\0';

	return i;
}

int redirect_output_streams(char *filename)
{
	mode_t mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fd == -1) {
		LOG_ERR("%s", strerror(errno));
		return -1;
	}
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);
	return 0;
}

int redirect(char **tokens, int *no_tokens)
{
	int counter = 0;
	for (int i = 0; i < *no_tokens; i++) {
		if (strncmp(tokens[i], ">\0", 2) == 0)
			counter++;
		if (counter > 1) {
			LOG_ERR("Only one \">\" symbol is allowed per command");
			return -1;
		}
	}
	if (counter == 0)
		return 0;
	if (*no_tokens >= 3 && strncmp(tokens[*no_tokens - 2], ">\0", 2) == 0) {
		if (redirect_output_streams(tokens[*no_tokens - 1]) == -1)
			return -1;
		tokens[*no_tokens - 2] = NULL;
		*no_tokens -= 2;
	} else {
		LOG_ERR("\">\" symbol must followed by a filepath");
		return -1;
	}
	return 0;
}

int execute_program(char **tokens, int no_tokens, int _wait)
{
	pid_t pid = fork();
	if (pid == 0) {
		// child process
		if (redirect(tokens, &no_tokens) == -1)
			goto end;
		for (int i = 0; i < no_paths; i++) {
			int path_len = strlen(paths[i]);
			int len = path_len + strlen(tokens[0]);
			char *path = malloc(sizeof(char) * len + 1);
			memcpy(path, paths[i], path_len);
			memcpy(path + path_len, tokens[0], len - path_len);
			path[len] = '\0';

			if (access(path, F_OK | X_OK) == 0) {
				free(tokens[0]);
				tokens[0] = path;
				execv(path, tokens);
				break;
			}
		}

		LOG_ERR("%s", strerror(errno));
end:;
		_exit(1);
	} else if (pid > 0) {
		if (_wait)
			waitpid(pid, NULL, 0);
		else
			return pid;
	} else {
		LOG_ERR("%s", strerror(errno));
	}
	return 1;
}

int execute_sequential_program(char **tokens, int no_tokens)
{
	return execute_program(tokens, no_tokens, 1);
}

int execute_background_program(char **tokens, int no_tokens)
{
	return execute_program(tokens, no_tokens, 0);
}

void process_line(char **tokens, int no_tokens)
{
	int pids[MAXCMDS], n = 0;
	char **tmp = tokens;
	for (int i = 0; i <= no_tokens; i++) {
		if (i == no_tokens && tmp != tokens + i) {
			execute_sequential_program(tmp, tokens + i - tmp);
		} else if (tokens[i] != NULL &&
			   strncmp(tokens[i], "&", 2) == 0) {
			tokens[i] = NULL;
			pids[n++] = execute_background_program(
				tmp, tokens + i - tmp);
			tmp = tokens + i + 1;
		}
	}
	for (int i = 0; i < MAXTOKENS; i++) {
		if (pids[i] > 0)
			waitpid(pids[i], NULL, 0);
	}
}

void start(FILE *stream)
{
	char *cmd = NULL;
	char **tokens = malloc(sizeof(char *) * MAXTOKENS);
	if (tokens == NULL)
		return;
	int no_tokens = 0;
	size_t cmd_len, len;

	while (true) {
		if (stream == stdin)
			printf("witsshell>");
		if ((cmd_len = getline(&cmd, &len, stream)) == -1 ||
		    cmd[cmd_len - 1] != '\n')
			goto end;
		cmd[--cmd_len] = '\0';
		if ((no_tokens = tokenize(tokens, MAXTOKENS, cmd, cmd_len)) ==
		    0)
			goto end;
		token_preprocessor(tokens, no_tokens);

		if (!built_in(tokens, no_tokens))
			process_line(tokens, no_tokens);

		free_tokens(tokens, no_tokens);
		free(cmd);
		cmd = NULL;
	}
end:;
	free_tokens(tokens, no_tokens);
	free(tokens);
	if (cmd != NULL)
		free(cmd);
}

void interactive_start()
{
	start(stdin);
}

void batch_start(char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		LOG_ERR("%s", strerror(errno));
		exit(1);
	}
	start(file);
	fclose(file);
}

int main(int argc, char **argv)
{
	if (argc >= 3) {
		fprintf(stderr, "Usage: ./witsshell [batchfile]\n");
		exit(1);
	}
	paths = malloc(sizeof(char *));
	paths[0] = malloc(sizeof(char) * 6);
	memcpy(paths[0], "/bin/", 6);
	no_paths = 1;

	if (argc == 1) {
		interactive_start();
	} else {
		batch_start(argv[1]);
	}

	free_paths();
	return 0;
}
