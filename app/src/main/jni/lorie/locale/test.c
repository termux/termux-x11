#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <execinfo.h>
#include <signal.h>
#include "keymaps.h"
void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void getSymLayout1(int keyCode, char* sym) {
	int i;
	for (i=0; lorie_keymaps[i]; i++) {
		if (lorie_keymaps[i]->keysyms[keyCode - KEYCODE_MIN].normal[0] == sym[0]) {
			printf("aasdasd\n");
			return;
		}
	}
	printf("lorie_keymaps[i]->keysyms[%d].normal[0] = %c; sym[0] = %c\n", 
		keyCode + KEYCODE_MIN,
		lorie_keymaps[0]->keysyms[keyCode - KEYCODE_MIN].normal[0], 
		sym[0]);
	printf("aqweqweqwe\n");
}

char* getSymLayout(int keyCode, char* sym) {
	int i, j;
	for (i=0; lorie_keymaps[i]; i++) {
		for (j=0; j<(KEYCODE_MAX-KEYCODE_MIN); j++) {
			if (!strcmp(sym, lorie_keymaps[i]->keysyms[j].normal)) return lorie_keymaps[i]->name;
		}
	}
	for (i=0; lorie_keymaps[i]; i++) {
		for (j=0; j<(KEYCODE_MAX-KEYCODE_MIN); j++) {
			if (!strcmp(sym, lorie_keymaps[i]->keysyms[j].shift)) return lorie_keymaps[i]->name;
		}
	}
	return "unknown";
}

int main(void){
	signal(SIGSEGV, handler);
	
	char *teststrings[] = {"q", "w", "e", "1", "2", "3", "ф", "ы", "в", "ф", "א", "ת", "ה", NULL};
	for (int i=0; teststrings[i]; i++) {
		printf("sym: %s, layout: %s\n", teststrings[i], getSymLayout(0, teststrings[i]));
	}
}
