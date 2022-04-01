#include "yaw.h"

/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2022 Michael Czigler
 */

static int
parseoct(char *p, size_t n)
{
	int i = 0;

	while (*p < '0' || *p > '7') {
		++p;
		--n;
	}
	while (*p >= '0' && *p <= '7' && n > 0) {
		i *= 8;
		i += *p - '0';
		++p;
		--n;
	}
	return (i);
}

static int
is_end_of_archive(char *p)
{
	int n;
	for (n = 511; n >= 0; --n)
		if (p[n] != '\0')
			return (0);
	return (1);
}

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
verify_checksum(char *p)
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

static char *
get_path(char *url)
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
	return path;
}

static int
get_http_respcode(char *buffer)
{
	char proto[4096], descr[4096];
	int code = 0;
	if (sscanf(buffer, "%s %d %s", proto, &code, descr) < 2) {
		code = -1;
	}
	return code;
}

static void
package_untar(char *tar, char *path)
{
	char buff[512];
	size_t bytes_read;
	int f, filesize, a = open(tar, O_RDONLY);
	printf("Extracting from %s\n", path);
	for (;;) {
		bytes_read = read(a, buff, 512);
		if (bytes_read < 512) {
			fprintf(stderr,
			    "Short read on %s: expected 512, got %ld\n",
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
			bytes_read = read(a, buff, 512);
			if (bytes_read < 512) {
				fprintf(stderr,
				    "Short read on %s: Expected 512, got %ld\n",
				    path, bytes_read);
				return;
			}
			if (filesize < 512) {
				bytes_read = filesize;
			}	
			if (f != -1) {
				if (write(f, buff, bytes_read) != bytes_read)
				{
					fprintf(stderr, "Failed write\n");
					close(f);
					f = -1;
				}
			}
			filesize -= bytes_read;
		}
		if (f != -1) {	
			close(f);
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
			fprintf(stderr, "socket\n");
			continue;
		}
		err = connect(conn, p->ai_addr, p->ai_addrlen);
		if (err < 0) {
			close(conn);
			fprintf(stderr, "connect\n");
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	int fd = open("tarball", O_WRONLY | O_CREAT | O_TRUNC, 0640);	
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

pkg
package_read(char *name)
{
	int fd = 0;
	pkg package;
	return package;
}

pkg
package_create(char *name, char *source_url, char *version)
{
	char *host = source_url;
	char *path = get_path(host);
	if (err == 0) {
		package = malloc(sizeof(pkg_t));
		package->name = name;
		package->version = version;
		package->host = host;
		package->path = path;
		package->build = DEFAULT_BUILD;
		package->checksum = 0;
	}
	return package;
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
	printf("checksum: %d\n", package->checksum);
	printf("build:    %s\n", package->name);
	return err;
}

int
package_verify(pkg package)
{
	int err = 0;
	return err;
}

int
main(int argc, char **argv)
{
	int err = 0;
	if (argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
		case 'v':
			puts("Copyright (C) 2022, Michael Czigler");
			break;
		case 'n':
			pkg package = package_create(package, argv[2], argv[3], argv[4]);
			err += package_write(package);
			err += package_print(package);
			err += package_destroy(package);
			break;
		case 'c':
			pkg package = package_read(argv[2]);
			err += package_verify(package);
			err += package_destroy(package);
			break;
		default:
			puts("yaw -v|n|c|i|r|l");
			break;
		}
	}
	return err;
}
