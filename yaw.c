#include "yaw.h"

/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2022 Michael Czigler
 */

static void
create_dir(char *pathname, int mode)
{
	if (pathname[strlen(pathname) - 1] == '/') {
		pathname[strlen(pathname) - 1] = '\0';
	}	
	int r = mkdir(pathname, mode);
	if (r != 0) {
		char *p = strrchr(pathname, '/');
		if (p != NULL) {
			*p = '\0';
			create_dir(pathname, 0755);
			*p = '/';
			r = mkdir(pathname, mode);
		}
	}
	if (r != 0) {
		fprintf(stderr, "Could not create directory %s\n", pathname);
	}
}

static int
create_file(char *pathname, int mode)
{
	int	f = open(pathname, O_CREAT|O_RDWR|O_APPEND, 0640);
	if (f == -1) {
		char *p = strrchr(pathname, '/');
		if (p != NULL) {
			*p = '\0';
			create_dir(pathname, 0755);
			*p = '/';
			f = open(pathname, O_CREAT|O_RDWR|O_APPEND, 0640);
		}
	}
	return (f);
}

static int
verify_checksum(const char *p)
{
	int n, u = 0;
	for (n = 0; n < 512; ++n) {
		if (n < 148 || n > 155) {
			u += ((unsigned char *)p)[n];
		} else {
			u += 0x20;
		}
	}
	return (u == parseoct(p + 148, 8));
}

static int
get_path(char *url, char *path)
{
	int err = 0;
	char *u = url, *path = NULL;
	while (*u) {
		if (*u == '/') {
			*u++ == '\0';
			path = u;
			break;
		}
		u++;
	}
	if (*u == NULL) err = 1;
	return err;
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

//static void
//untar(FILE *a, const char *path)
static void
package_untar(char *tar, char *path)
{
	char buff[512];
	//FILE *f = NULL;
	size_t bytes_read;
	int filesize, a = open(tar, O_RDONLY;
	printf("Extracting from %s\n", path);
	for (;;) {
		//bytes_read = fread(buff, 1, 512, a);
		bytes_read = read(a, buff, 512);
		if (bytes_read < 512) {
			fprintf(stderr,
			    "Short read on %s: expected 512, got %d\n",
			    path, bytes_read);
			return;
		}
		if (is_end_of_archive(buff)) {
			printf("End of %s\n", path);
			return;
		}
		if (!verify_checksum(buff)) {
			fprintf(stderr, "Checksum failure\n");
			return;
		}
		filesize = parseoct(buff + 124, 12);
		switch (buff[156]) {
		case '1':
			printf(" Ignoring hardlink %s\n", buff);
			break;
		case '2':
			printf(" Ignoring symlink %s\n", buff);
			break;
		case '3':
			printf(" Ignoring character device %s\n", buff);
				break;
		case '4':
			printf(" Ignoring block device %s\n", buff);
			break;
		case '5':
			printf(" Extracting dir %s\n", buff);
			create_dir(buff, parseoct(buff + 100, 8));
			filesize = 0;
			break;
		case '6':
			printf(" Ignoring FIFO %s\n", buff);
			break;
		default:
			printf(" Extracting file %s\n", buff);
			f = create_file(buff, parseoct(buff + 100, 8));
			break;
		}
		while (filesize > 0) {
			//bytes_read = fread(buff, 1, 512, a);
			bytes_read = read(a, buff, 512);
			if (bytes_read < 512) {
				fprintf(stderr,
				    "Short read on %s: Expected 512, got %d\n",
				    path, bytes_read);
				return;
			}
			if (filesize < 512) {
				bytes_read = filesize;
			}	
			//if (f != NULL) {
			if (f != -1) {
				//if (fwrite(buff, 1, bytes_read, f) != bytes_read)
				if (write(f, buff, bytes_read) != bytes_read)
				{
					fprintf(stderr, "Failed write\n");
					fclose(f);
					//f = NULL;
					f = -1;
				}
			}
			filesize -= bytes_read;
		}
		//if (f != NULL) {
			//fclose(f);
		if (f != -1) {	
			close(f);
			//f = NULL;
			f = -1;
		}
	}
}

int
package_download(pkg package)
{
	if (chdir(package->name) < 0) puts("directory does not exist");
	int err, conn;
	struct addrinfo *res, hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM
	};
	if ((err = getaddrinfo(package->host, "80", &hints, &res)) != 0) {
		perror("getaddrinfo");
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return 3;
	}
	struct addrinfo *p;
	for (p = res; p != NULL; p = p->ai_next) {
		conn = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (conn < 0) {
			//perror("socket");
			fprintf(stderr, "socket\n");
			continue;
		}
		err = connect(conn, p->ai_addr, p->ai_addrlen);
		if (err < 0) {
			close(conn);
			//perror("connect");
			fprintf(stderr, "connect\n");
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	int fd = open("./tarball", O_WRONLY | O_CREAT | O_TRUNC, 0640);	
	char buf[4096];
	strcpy(buf, "GET /");
	if (package->path) {
		strncat(buf, package->path, sizeof(buf) - strlen(buf) - 1);
  	}
  	strncat(buf, " HTTP/1.0\r\nHost: ", sizeof(buf) - strlen(buf) - 1);
	strncat(buf, package->host, sizeof(buf) - strlen(buf) - 1);
  	strncat(buf, "\r\n\r\n", sizeof(buf) - strlen(buf) - 1);
	int tosent = strlen(buf), header = 1, nrecvd;
	int nsent = send(conn, buf, tosent, 0);
	if (nsent != tosent) {
		return 5;
	}
	printf("[tarball] download started\n");
	while ((nrecvd = recv(conn, buf, sizeof(buf), 0))) {
		char *ptr = buf;
		if (header) {
			ptr = strstr(buf, "\r\n\r\n");
			if (!ptr) continue;
			int rcode = get_http_respcode(buf);
			if (rcode / 100 != 2) {
				return rcode / 100 * 10 + rcode % 10;
			}	
			header = 0;
			ptr += 4;
			nrecvd -= ptr - buf;
		}
		printf("bytes received: %d\n", nrecvd);
		write(fd, ptr, nrecvd);
	}
	printf("[tarball] download complete\n");
	close(fd);
	close(conn);
	return 0;
}

int
package_extract(pkg package)
{
	int err = 0;
	return err;
}

int
package_write(pkg package)
{
	int err = 0, fd = 0;
	return err;
}

int
package_read(char *name)
{
	int err = 0, fd = 0;
	return err;
}

int
package_create(pkg package, char *name, char *source_url, char *version)
{
	char *path, *host = source_url;
	int err = get_path(host, path);
	if (ret == 0) {
		package = malloc(sizeof(pkg_t));
		package->name = name;
		package->version = version;
		package->host = host;
		package->path = path;
		package->build = DEFAULT_TEMPLATE;
		package->checksum = 0;
	}
	return err;
}

int
package_destroy(pkg package)
{
	int err = 0;
	free(package);
	return err;
}

int
package_print(pkg package)
{
	int err = 0;
	puts("[package details]");
	printf("name:     %s\n", package->name);
	printf("host:     %s\n", package->host);
	printf("path:     %s\n", package->path);
	printf("version:  %s\n", package->version);
	printf("checksum: %ld\n", package->name);
	printf("build:    %s\n", package->name);
	return err;
}

int
main(int argc, char **argv)
{
	int err = 0;
	if (argc > 1 && argv[1][0] == '-') {
		pkg package;
		switch(argv[1][1]) {
		case 'v':
			puts('Copyright (C) 2022, Michael Czigler')
			break;
		case 'n':
			err = package_create(package, argv[2], argv[3], argv[4]);
			break;
		case 'c':
			err = package_verify(package, argv[2]);
			break;
		default:
			puts("yaw -v|n|c|i|r|l");
			break;
		}
	}
	return err;
}
