#include <stdio.h>

static void __attribute__((__constructor__)) load() {
	puts("libxcb is loaded from apk...\n");
}
