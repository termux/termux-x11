#include <jni.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define unused __attribute__((unused))
#define used __attribute__((used))

JNIEXPORT jint JNICALL used
Java_com_eltechs_axs_Mcat_startVtest(unused JNIEnv *env, unused jobject thiz) {
    int pid = fork();
    switch (pid) { // NOLINT(hicpp-multiway-paths-covered)
        case 0: {
			const char* packages[] = {
				"com.antutu.ABenchMark",
				"com.ludashi.benchmark",
				"com.eltechs.ed",
				NULL
			};
			
			int fd = open("/storage/emulated/0/android-virtioGPU.txt", O_WRONLY | O_CREAT | O_TRUNC, 644);
			dup2(fd, 1);
			dup2(fd, 2);
			
			setenv("TERM", "xterm-256color", 1);
			setenv("LC_ALL", "zh_CN.utf8", 1);
			setenv("LANG", "zh_CN.utf8", 1);
			setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games", 1);
			setenv("HOME", "/root", 1);
			
			for (int i=0; packages[i]; i++) {
				char *root, *tmp, *script;
				asprintf(&root, "/data/data/%s/files", packages[i]);
				asprintf(&tmp, "/data/data/%s/files/image/tmp", packages[i]);
				asprintf(&script, "/data/data/%s/files/image/opt/start.sh", packages[i]);
				setenv("ROOT", root, 1);
				setenv("PROOT_TMP_DIR", tmp, 1);
				
				if (access(script, F_OK) == 0) {
					printf("Executing %s\n", script);
					execl("/system/bin/sh", "sh", script, NULL);
					printf("Error executing %s: %s\n", script, strerror(errno));
				} else printf("Script %s is not accessible or does not exist\n", script);
			}
            return -1;
		}
        default:
            return pid;
    }
}

JNIEXPORT void JNICALL used
Java_com_eltechs_axs_Mcat_stopVtest(unused JNIEnv *env, unused jobject thiz, jint pid) {
    killpg(pid, SIGINT);
}
