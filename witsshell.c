#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFERSIZE 1024
#define MAXTOKENS 64

#define LOG_ERR(fmt, ...)                                             \
	fprintf(stderr, "%s:%d (%s) - " fmt "\n", __FILE__, __LINE__, \
		__func__ __VA_OPT__(, ) __VA_ARGS__)
#define IS_QUOTE(chr) chr == '"' || chr == '\'' || chr == '`'
#define IS_PARALLEL(chr) chr == '&'
#define IS_REDIRECTION(chr) chr == '>'

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
		free(tokens[i]);
	free(tokens);
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
			if (strncmp(token, " ", 1) != 0)
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

void execute_program(char **tokens, int no_tokens, int _wait)
{
	pid_t pid = fork();
	if (pid == 0) {
		// child process
		execv(tokens[0], tokens);

		LOG_ERR("%s", strerror(errno));
		_exit(errno);
	} else if (pid > 0) {
		if (_wait)
			waitpid(pid, NULL, 0);
	} else {
		LOG_ERR("%s", strerror(errno));
	}
}

void execute_sequential_program(char **tokens, int no_tokens)
{
	execute_program(tokens, no_tokens, 1);
}

void execute_background_program(char **tokens, int no_tokens)
{
	execute_program(tokens, no_tokens, 0);
}

void interactive_start()
{
	char buffer[BUFFERSIZE];
	char **tokens = malloc(sizeof(char *) * MAXTOKENS);
	int cmd_len, no_tokens;

	printf("witsshell>");
	cmd_len = get_input_string(buffer, BUFFERSIZE);

	if ((no_tokens = tokenize(tokens, MAXTOKENS, buffer, cmd_len)) == 0)
		goto end;
	token_preprocessor(tokens, no_tokens);
end:;
	free_tokens(tokens, no_tokens);
}

int main(int argc, char **argv)
{
	if (argc >= 3) {
		fprintf(stderr, "Usage: ./witsshell [batchfile]\n");
		return 1;
	}

	if (argc == 1) {
		interactive_start();
	}

	printf("\n\e[0;32m======\e[0m\nGoodbye\n\e[0;32m======\e[0m\n");
	return 0;
}
