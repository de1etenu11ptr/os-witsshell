#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFERSIZE 1024
#define MAXTOKENS 64

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

	return no_tokens;
}

int get_input_string(char *buffer, int buffersize)
{
	int i = 0;
	char c;

	while (i < buffersize - 1) {
		/* Consider the case where a multiline string is being entered that has
		 * been introduced by some opening quote
		 */
		if ((c = fgetc(stdin)) == '\n')
			break;
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

	no_tokens = tokenize(tokens, MAXTOKENS, buffer, cmd_len);
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
