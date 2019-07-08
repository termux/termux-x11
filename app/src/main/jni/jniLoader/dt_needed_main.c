#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include "log.h"
#ifdef __ANDROID__
#include <jni.h>
int System_load(JNIEnv *env, char *filename);
#endif

#ifndef __ANDROID__
#define __USE_GNU
#include <dlfcn.h>
#include <execinfo.h>
int trace_funcs = 0;
void __attribute__((no_instrument_function))
__cyg_profile_func_enter (void *func,  void *caller) {
	if (!trace_funcs) return;
  Dl_info info;
  if (dladdr(func, &info))
    LOGD ("enter [%s] %s", (info.dli_fname) ? info.dli_fname : "?", info.dli_sname ? info.dli_sname : "?");
}
void __attribute__((no_instrument_function))
__cyg_profile_func_exit (void *func,  void *caller) {
	if (!trace_funcs) return;
  Dl_info info;
  if (dladdr(func, &info))
    LOGD ("leave [%s] %s", (info.dli_fname) ? info.dli_fname : "?", info.dli_sname ? info.dli_sname : "?");
}
#endif
#define static // backtrace do not report static function names


#define bit64
#include "dt_needed.c"
#undef bit64
#include "dt_needed.c"

char ** str_split(char *s, char *delim) { 
	char *strings[128];
	char *pch, **result, *str = strdup(s);
	int i=0, j=0;
	pch = strtok (str,delim);
	while (pch != NULL) {
		strings[i] = strdup(pch);
		pch = strtok(NULL, ":");
		i++;
	}
	result = calloc(i+1, sizeof(char*));
	if (!result) {
		LOGE("str_split: Failed to allocate result array: %s", strerror(errno));
		return NULL; 
	}
	
	for (j=0; j<i; j++)
		result[j] = strings[j];
	result[i] = NULL;
	
	free(str);
    return result;
}
char ** dlneeds(char *filename) {
    Elf32_Ehdr hdr;
    int fd = open( filename, O_RDONLY );
    if( fd < 0 ) {
        LOGE("Cannot open %s: %s", filename, strerror( errno ) );
        return NULL;
    }

    if( read( fd, &hdr, sizeof( Elf32_Ehdr ) ) < 0 ) {
        LOGE("%s: Cannot read elf header: %s", filename, strerror( errno ) );
        return NULL;
    }
    
    lseek(fd, 0, SEEK_SET);
    if (hdr.e_ident[EI_CLASS] == ELFCLASS32) return dlneeds32(fd);
    if (hdr.e_ident[EI_CLASS] == ELFCLASS64) return dlneeds64(fd);
    
    //Shoult never reach it.
    return NULL;
}

char *find_realpath(char *filename) {
	if (!filename) return NULL;
	
	char *result = NULL;
    int i;
    char path[1024];
    char *LD_LIBRARY_PATH="/data/data/com.termux/files/usr/lib:/system/lib"; // "/usr/lib:/usr/lib/x86_64-linux-gnu";
    char **paths = str_split(strdup(LD_LIBRARY_PATH), ":");
    if (paths == NULL) goto done;
    
    for (i=0;paths[i]!=NULL;i++) {
		sprintf(path, "%s/%s", paths[i], filename);
		if(access(path, F_OK) != -1) {
			result = strdup(path);
			break;
		}
	}

done:
	if (paths) {
		for(i=0;paths[i]!=NULL;i++)
			free(paths[i]);

		free(paths);
	}

	return result;
}

static char *dl_skips[] = {
	"libc.so",
	"libdl.so",
	NULL,
};

int dl_skipping(char* bname) {
	int i;
	for (i=0; dl_skips[i]!=NULL; i++) {
		if (strcmp(dl_skips[i], bname)==0)
			return 1;
	}
	return 0;
}

int android_dlopen(JNIEnv *env, char* filename) {
    int i, j, r;
    char *bname = basename(filename);
	char *realpath = find_realpath(bname);
	if (!realpath) {
		LOGE("%s is not found", filename);
		return 1;
	}
    char **dl_needs = dlneeds(realpath);
    if (dl_needs)
    for (i=0;;i++) {
		int skipping = 0;
		if (dl_needs[i] == NULL) break;
		if (dl_skipping(filename)) break;
		if (!skipping) {
			r = android_dlopen(env, dl_needs[i]);
			if (r) {
			    LOGE("Failed to dlopen %s", dl_needs[i]);
			    break;
			}
		}
	}
	free(realpath);

	return System_load(env, realpath);
}

#ifndef __ANDROID__
int main(int argc, char **argv) {
	if (getenv("TRACE_FUNCS") != NULL) trace_funcs = 1;
    if( argc < 2 ) {
        LOGE("Usage %s <binary>", argv[0]);
        return 1;
    }

    android_dlopen(argv[1]);
    return 0;
}
#endif

#ifdef __ANDROID__
JavaVM *jvm = NULL;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	jvm = vm;
	return JNI_VERSION_1_4;
}

int System_load(JNIEnv *env, char *filename) {
	jclass System = (*env)->FindClass (env, "java/lang/System");
	if(System == NULL) {
		LOGE("ERROR: class java.land.System is not found!");
	}

	jmethodID load = (*env)->GetStaticMethodID(env, System, "load","(Ljava/lang/String;)V");
	if (load == NULL) {
		LOGE("ERROR: method System.load is not found!");
	}
	
	jstring file = (*env)->NewStringUTF(env, filename);
	if (file == NULL) {
		LOGE("failed to create jstring");
	}

	(*env)->CallStaticVoidMethod(env, System, load, file);
	(*env)->DeleteLocalRef(env, file);
	(*env)->DeleteLocalRef(env, System);


    jboolean flag = (*env)->ExceptionCheck(env);
    if (flag) {
        jthrowable e = (*env)->ExceptionOccurred(env);
        (*env)->ExceptionClear(env);
        jclass clazz = (*env)->GetObjectClass(env, e);
        jmethodID getMessage = (*env)->GetMethodID(env, clazz, "getMessage", "()Ljava/lang/String;");
        jstring message = (jstring)(*env)->CallObjectMethod(env, e, getMessage);

        const char *mstr = (*env)->GetStringUTFChars(env, message, NULL);
        LOGE("%s", mstr);
        (*env)->ReleaseStringUTFChars(env, message, mstr);

        (*env)->DeleteLocalRef(env, message);
        (*env)->DeleteLocalRef(env, clazz);
        (*env)->DeleteLocalRef(env, e);

        return 1;
    }
    return 0;
}




JNIEXPORT void JNICALL
Java_com_termux_wtermux_jniLoader__1loadLibrary(JNIEnv *env, jclass type, jstring name_) {
	char *name = (char*) (*env)->GetStringUTFChars(env, name_, 0);
    android_dlopen(env, name);
	(*env)->ReleaseStringUTFChars(env, name_, name);
}
#endif
