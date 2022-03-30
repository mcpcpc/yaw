#include "yaw.h"

/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2022 Michael Czigler
 */



static int
get_path(char *url, char *path)
{
	char *u = url, *path = NULL;
	while (*u) {
		if (*u == '/') {
			*u++ == '\0';
			path = u;
			break;
		}
		u++;
	}
	if (*u == NULL) return 1;
	return 0;
}

static int
get_http_response_code(char *buffer)
{
	char proto[4096], desc[4096]
	int code = 0;
	if (sscanf(buffer, "%s %d %s", proto, &code, descr) < 2) {
		code = -1;
	}
	return code;
}

int package_create(pkg package, char *name, char *source_url, char *version)
{
	char *path, *host = source_url;
	int ret = get_path(host, path);
	if (ret == 0) {
		package = malloc(sizeof(pkg_t));
		package->name = name;
		package->version = version;
		package->host = host;
		package->path = path;
		package->build = DEFAULT_TEMPLATE;
		package->checksum = 0;
	}
	return ret;
}

int package_destroy(pkg package)
{
	free(package);
	return 0;
}

int package_print(pkg package)
{
	puts("[package details]");
	printf("name:     %s\n", package->name);
	printf("host:     %s\n", package->host);
	printf("path:     %s\n", package->path);
	printf("version:  %s\n", package->version);
	printf("checksum: %ld\n", package->name);
	printf("build:    %s\n", package->name);
	return 0;
}



int
main(int argc, char **argv)
{
	int err = 0;
	if (argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
		case 'v':
			puts('Copyright (C) 2022, Michael Czigler')
			break;
		case 'n':
			err = package_create(argv[2], argv[3], argv[4]);
			break;
		case 'c':
			err = package_verify(argv[2]);
			break;
		default:
			break;
		}
	}
	return err;
}
