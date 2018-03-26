// Copyright 2018, Jeffrey E. Bedard
#include <arpa/inet.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
enum { RFC868ADJ = 2208988800,
	CORRECTION = 123010304,
	CLOCKDADJ = CORRECTION - RFC868ADJ,
	FLAG_SET_TIME = 1};
static int clockd_cleanup_fd;
static uint8_t clockd_flags;
void signal_cb(int sig)
{
	(void)sig;
	close(clockd_cleanup_fd);
}
// simple RFC 868 client/server
static int get_socket(void)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket()");
		exit(1);
	}
	clockd_cleanup_fd = fd;
	signal(SIGINT, &signal_cb);
	signal(SIGKILL, &signal_cb);
	signal(SIGTERM, &signal_cb);
	return fd;
}
static time_t get_unix_time(int fd)
{
	int32_t t;
	uint8_t * bytes = (uint8_t *)&t;
	read(fd, bytes, 4);
	t = ntohl(t);
	return t - CLOCKDADJ;
}
static void set_unix_time(time_t t)
{
	struct timeval tv = {t, 0};
	puts("setting time...");
	if (settimeofday(&tv, NULL) < 0)
		perror("settimeofday()");
}
static void print_time(const time_t t)
{
	char buf[26];
	printf("time:  %s\n", ctime_r(&t, buf));
}
static int client(const char * addr)
{
	printf("opening %s\n", addr);
	int fd = get_socket();
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(37);
	inet_pton(AF_INET, addr, &sa.sin_addr);
	if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		perror("connect()");
		exit(1);
	}
	time_t t = get_unix_time(fd);
	print_time(t);
	if (clockd_flags & FLAG_SET_TIME)
		set_unix_time(t);
	return 0;
}
static void print_868_time(int fd)
{
	// ADJ converts the time to be relative to 01 JAN 1900,
	// rather than 01 JAN 1970.
	enum {SZ = 20};
	uint32_t t = time(NULL) + CLOCKDADJ;
	printf("sending %d to peer", t);
	t = htonl(t);
	uint8_t * bytes = (uint8_t *)&t;
	write(fd, bytes, 4);
}
static int server()
{
	puts("clockd server starting");
	const int fd = get_socket();
	struct sockaddr_in a;
	{ // reuse socket if the kernel still has it
		int optval;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			&optval, sizeof(int));
	}
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_ANY);
	a.sin_port = htons(37);
	if (bind(fd, (struct sockaddr *)&a, sizeof(struct sockaddr)) < 0) {
		perror("bind()");
		return 1;
	}
	listen(fd, 16); // accept up to 16 pending connections
	struct sockaddr c_adr;
	socklen_t len;
	for (;;) {
		int c_fd = accept(fd, &c_adr, &len);
		if (c_fd < 0) {
			perror("accept()");
			close(fd);
			return 1;
		} else {
			print_868_time(c_fd);
			close(c_fd);
		}
	}
	close(fd);
	return 0;
}
int main(int argc, char ** argv)
{
	char opt, opts[] = "Sc:s";
	while ((opt = getopt(argc, argv, opts)) != -1) {
		switch(opt) {
		case 'S': // set time with client
			clockd_flags |= FLAG_SET_TIME;
			break;
		case 'c': // optarg is the ip address
			exit(client(optarg));
		case 's': // run in server mode
			exit(server());
		default: // display usage
			fprintf(stderr, "%s -[%s]\n", argv[0], opts);
			exit(1);
		}
	}
	return 0;
}
