#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int get_input_string(char *buffer, int buffersize)
{
	int i = 0, is_quoted = 0;
	char c;

	while (i < buffersize - 1) {
		/* Consider the case where a multiline string is being entered that has
		 * been introduced by some opening quote
		 */
		i++;
	}

	return i;
}

void interactive_start()
{
	printf("witsshell>");

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

	return 0;
}
