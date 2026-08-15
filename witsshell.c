#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define BUFFERSIZE 1024
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
	char buffer[BUFFERSIZE], char **tokens;
	int cmd_len;

	printf("witsshell>");
	fflush(stdout);
	cmd_len = get_input_string(buffer, BUFFERSIZE);

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
