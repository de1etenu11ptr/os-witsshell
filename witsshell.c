#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFERSIZE 1024
#define MAXTOKENS 64

#define LOG_ERR(fmt, ...)                                             \
	fprintf(stderr, "%s:%d (%s) - " fmt "\n", __FILE__, __LINE__, \
		__func__ __VA_OPT__(, ) __VA_ARGS__)

int tokenize(char **tokens, int max_tokens, char *cmd, int cmd_len)
{
	char *token = cmd, quote;
	int i = 0, is_quoted = 0, no_tokens = 0;

	while (i <= cmd_len && no_tokens < max_tokens) {
		if (is_quoted) {
			if (cmd[i] == quote && cmd[i - 1] != '\\') {
				is_quoted = 0;
			}
		} else if (cmd[i] == '"' || cmd[i] == '\'' || cmd[i] == '`') {
			is_quoted = 1;
			quote = cmd[i];
		} else if (cmd[i] == ' ' || cmd[i] == '\0') {
			cmd[i] = '\0';
			tokens[no_tokens++] = token;
			token = cmd + i + 1;
		}
		i++;
	}
	if (is_quoted) {
		LOG_ERR("Failed to tokenize due to the unmatched quote \"%c\"",
			quote);
		return 0;
	}

	return no_tokens;
}

void token_preprocessor(char **tokens, int no_tokens)
{
	for (int i = 0; i < no_tokens; i++) {
		char *token = tokens[i];
		int read = 0, write = 0;

		while (token[read]) {
			if (token[read] == '"' || token[read] == '\'' ||
			    token[read] == '`') {
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
	int i = 0, is_quoted;
	char c, quote;

	while (i < buffersize - 1) {
		if ((c = fgetc(stdin)) == '\n' && !is_quoted)
			break;
		else if (is_quoted && c == quote && buffer[i - 1] != '\\') {
			is_quoted = 0;
		} else if (c == '"' || c == '\'' || c == '`') {
			is_quoted = 1;
			quote = c;
		} else if (c == '\n') {
			buffer[i++] = ' ';
			continue;
		}
		buffer[i++] = c;
	}
	buffer[i] = '\0';

	return i;
}

void interactive_start()
{
	char buffer[BUFFERSIZE];
	char **tokens = malloc(sizeof(char *) * MAXTOKENS);
	int cmd_len, no_tokens;

	printf("witsshell>");
	fflush(stdout);
	cmd_len = get_input_string(buffer, BUFFERSIZE);

	if ((no_tokens = tokenize(tokens, MAXTOKENS, buffer, cmd_len)) == 0)
		return;
	token_preprocessor(tokens, no_tokens);
	for (int i = 0; i < no_tokens; i++) {
		printf("%d: %s\n", i, tokens[i]);
	}
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

	printf("\n");
	return 0;
}
