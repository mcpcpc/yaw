#ifndef __YAW_H
#define __YAW_H

#define _POSIX_C_SOURCE 200809L
#define DEFAULT_BUILD "make\nmake install"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>

typedef struct PACKAGE {
	char    *name;
	char    *host;
	char    *path;
	char    *version;
	char    *build;
	uint32_t checksum;
} pkg_t, *pkg;

int package_create(pkg package, char *name, char *source_url, char *version);
int package_destroy(pkg package);
int package_download(pkg package);
int package_verify(pkg package);
int package_print(pkg ppackage);

#endif
