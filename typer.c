#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
//#include <unac.h>
#include <locale.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>

#define sample_text "Nessler cylinders are used for colorimetric analysis, such as APHA color. The color of the substance contained in a Nessler cylinder is visually compared with the model. The tubes are often used to carry out a series of calibrations of solutions of increasing concentrations, which functions as a comparative scale. To minimize differences in the subjective impression of the color of the solution of the substance to be analyzed, cylinders of a series should have the same characteristics - height, diameter and thickness of glass."

#define CH_PER_LINE 50

void change_term(void);
void reset_term(void);
size_t num_bytes(const char *);
unsigned int e_read(void);
unsigned int int_char_read(void);
void error_made(unsigned char*, size_t, size_t, bool);
unsigned int num_errors(unsigned char*, size_t);

struct termios *orig_info = NULL;
int numlines = 0;
int line = 0;

int main(void) {
	struct timespec s_time = {0, 0}, e_time;
	int characters = 0;
	unsigned int c;
	char editor[1024] = {0};
	char *endpos = editor;
	size_t numchars;
	FILE *fp;
	int errors = 0;
//	size_t tmplen = 3;
//	char *tmp = malloc(tmplen);
	bool backspace = false;
	char *tmpstr = NULL;

	unsigned int *line_len;
	size_t actual_size = 0;
	unsigned char *incorrect;
	int line_pos = 0;
	size_t chars_typed = 0;

	signal(SIGINT, exit);

	setlocale(LC_ALL, "");
	atexit(reset_term);
	change_term();

	//printf("\033[H\033[J");

	fp = popen("/usr/bin/python3 ./wiki.py", "r");
	if (NULL == fp) {
		fprintf(stderr, "Failed to run wiki.py\n");
		exit(1);
	}

	if (NULL == fgets((char *) editor, sizeof editor, fp)) {
		fprintf(stderr, "A problem occurred\n");
		exit(1);
	}
	pclose(fp);

	for (numchars = 0; editor[numchars] != 0; ++numchars);

	for (unsigned int i = CH_PER_LINE; i < numchars; i += CH_PER_LINE) {
		unsigned int tmp = i;
		while (!isspace(editor[++tmp]) && editor[tmp] != '\0');
		editor[tmp] = '\n';
		numlines++;
	}
	line_len = malloc((numlines + 1) * sizeof *line_len);

	size_t llen = 0;
	for (unsigned int i = 0; i < numchars; ++i) {
		int clen = num_bytes(editor + numchars);
		if (clen > 0)
			clen--;
		actual_size++;
		llen++;
		if ('\n' == editor[i]) {
//			if (line >= numlines) {
//				printf("line was %d, numlines was %d", line, numlines);
//				exit(1);
//			}
			line_len[line] = llen;
			llen = 0;
			line++;
		}
		i += clen;
	}
	numlines = line;
	line = 0;

	incorrect = malloc(actual_size / 8 + 1);
	memset(incorrect, 0, actual_size / 8 + 1);

	printf("\e[36;1m%s\e[0m\e[%dA\r", (char *) editor, numlines);
	fflush(stdout);

	/*// TESTING
	int col = 1;
	int l = 1;
	// TESTING*/

	while (endpos - editor < (signed) strlen((char *) editor)) {
		if (tmpstr) {
			free(tmpstr);
			tmpstr = NULL;
		}

		clock_gettime(CLOCK_REALTIME, &e_time);
		if (s_time.tv_sec != 0 && e_time.tv_sec - s_time.tv_sec >= 120) {
			printf("\e[%dB\r", numlines - line + 1);
			printf("\nTime out\n");
			break;
		}
		
		//c = getchar();
		//scanf("%lc", &c);
		c = int_char_read();
		// start the clock once user starts typing
		if (s_time.tv_sec == 0) clock_gettime(CLOCK_REALTIME, &s_time);

		chars_typed++;

//		if (c == 127)
//			continue;

		// clen is the actual length of the character currently in endpos
		size_t clen = num_bytes(endpos);

		//size_t clen = num_bytes(endpos);
		if (clen == 0) clen = 1;
		tmpstr = malloc(clen + 1);
		memcpy(tmpstr, endpos, clen);
		tmpstr[clen] = '\0';

		if (backspace) {
			backspace = false;
			if (tmpstr[0] != '\n')
				printf("\e[36;1m%s\e[0m\b", tmpstr);
			else
				printf(" \b");
			fflush(stdout);
		}

		// full character at endpos stored in an int for comparison
		unsigned int cnew = 0;
		cnew |= ((unsigned int) *endpos) & 0xff;
		unsigned int i = 0;
		while (++i < clen) {
			cnew <<= 8;
			cnew |= endpos[i];
		}
		//for (cnew = 0; ((c >> cnew * 8) & 0x00ffffff) != 0; cnew++);

		if (c == 127 && endpos > editor) {
			printf("\b");
			endpos -= clen;
			line_pos--;
			error_made(incorrect, actual_size / 8 + 1, chars_typed, false);
			chars_typed--;
			if (line_pos < 0) {
				line--;
				line_pos = line_len[line];
				printf("\r\e[1A\e[%dC", line_pos - 1);
			}
			backspace = true;
			fflush(stdout);
			continue;
		}

		line_pos++;

		if (c == cnew || (isspace(c) && isspace(cnew))) {
			printf("%s", tmpstr);
			fflush(stdout);
			characters++;
			/*// TESTING
			col++;
			// TESTING*/
			if ('\n' == *endpos) {
				//printf("\n");
				line++;
				line_pos = 0;
				/*// TESTING
				col = 0;
				l++;
				// TESTING*/
			}
			endpos += clen;
			continue;
		} 

		//unac_string("UTF-8", endpos, clen, &tmp, &tmplen);
		/*// TESTING
		printf("\r\x1b[0m\x1b[%dB", numlines);
		printf("%s %s", tmp, tmpstr);
		printf("\x1b[0m\x1b[%d;%dH", l, col);
		// TESTING*/
		//if (c == (unsigned) *tmp && *tmp != *endpos) {
		//	printf("%s", tmpstr);
		//	endpos += clen;
		//	continue;
		//}

		printf("\e[31m%s\e[0m", tmpstr);
		if ('\n' == *endpos) {
			line++;
			line_pos = 0;
		}
		endpos += clen;
		fflush(stdout);
		errors++;
		error_made(incorrect, actual_size / 8 + 1, chars_typed, true);

//		if (!iscntrl(c)) {
//			*endpos = c;
//			*++endpos = '\0';
//		}
	}
	printf("\n");
	clock_gettime(CLOCK_REALTIME, &e_time);

	printf("seconds: %ld\n", e_time.tv_sec - s_time.tv_sec);
	double accuracy = 1.0 - num_errors(incorrect, actual_size / 8 + 1) / (double) chars_typed;
	printf("accuracy: %.2lf\n", accuracy);
	double minutes = (e_time.tv_sec - s_time.tv_sec) / 60.0 + (e_time.tv_nsec - s_time.tv_nsec) / 60000000000.0;
//	int truechars = 0;
//	for (unsigned int i = 0; i < numchars; i++) {
//		if (editor[i] <= 0x80 && editor[i] >= 0xbf) {
//			i++;
//			continue;
//		}
//		truechars++;
//	}
	double wpm = (chars_typed / 5.0) / minutes;
	printf("words: %.2f, wpm: %.2lf\n", chars_typed / 5.0, wpm);
	printf("%.2lf%% * %.2lf wpm = %.2lf\n", accuracy, wpm, wpm * accuracy);

	return 0;
}

void error_made(unsigned char *errors, size_t err_len, size_t pos, bool error) {
	size_t davvero_pos = pos / 8;
	size_t piccolo_pos = pos % 8;
	errors[davvero_pos] &= ~(0x1 << piccolo_pos);
	errors[davvero_pos] |= error << piccolo_pos;
}

unsigned int num_errors(unsigned char *errors, size_t err_len) {
	unsigned int accum = 0;
	for (int i = 0; i < err_len; ++i) {
		// magic from: https://stackoverflow.com/questions/8871204/count-number-of-1s-in-binary-representation
		// and https://web.archive.org/web/20151229003112/http://blogs.msdn.com/b/jeuge/archive/2005/06/08/hakmem-bit-count.aspx
		unsigned int u = errors[i];
		unsigned int uCount = ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
		accum += ((uCount + (uCount >> 3)) & 030707070707) % 63;
	}
	return accum;
}

unsigned int int_char_read(void) {
	unsigned int c = e_read();
	char cc = (char) c;
	size_t clen = num_bytes(&cc);
	unsigned int i = 0;
	while (++i < clen) {
		c <<= 8;
		int tmp = e_read();
		c |= tmp;
	}
	return c;
}

unsigned int e_read(void) {
	static char tmp = '\0';
	int nread;
	while ((nread = read(STDIN_FILENO, &tmp, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) perror("read");
	}
	return ((unsigned int) tmp) & 0xff;
}

// returns the number of bytes a utf8 encoded character takes up
// returns -1 if there is an error
size_t num_bytes(const char *in) {
	// bitmasks to determine how many bytes the character is
	static const char masks[] =  {0x80, 0xe0, 0xf0, 0xf8};
	// after applying masks from above this is what the byte should be equal to
	static const char states[] = {0x00, 0xc0, 0xe0, 0xf0};

	char tmp = *in;
	for (int i = 0; i < 4; i++) {
		if ((tmp & masks[i]) == states[i])
			return i + 1;
	}
	return 0;
}

void change_term(void) {            
	if (NULL == orig_info) {
		// store original terminal state
		orig_info = malloc(sizeof *orig_info);
		// copy current terminal state
		struct termios info;
		tcgetattr(STDIN_FILENO, orig_info);
		info = *orig_info;
		// disable canonical mode and echo
		info.c_lflag &= ~(ICANON | ECHO);
		// idk lol
		info.c_cc[VMIN] = 1;
		info.c_cc[VTIME] = 0;
		// set terminal state to altered state
		tcsetattr(STDIN_FILENO, TCSANOW, &info);
	}
}

void reset_term(void) {
	if (NULL != orig_info) {
		tcsetattr(STDIN_FILENO, TCSANOW, orig_info);
		free(orig_info);
		orig_info = NULL;
		printf("\e[%dB\r\n", numlines - line + 1);
//		for (int i = 0; i < numlines; ++i)
//			printf("\n");
	}
}
