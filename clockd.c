// Copyright 2018, Jeffrey E. Bedard
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
enum { RFC868ADJ = 2208988800, CLOCKD_BUFSZ = 15,
	CORRECTION = 123010304,
	CLOCKDADJ = CORRECTION - RFC868ADJ,
	FLAG_SET_TIME = 1, FLAG_FORK = 2};
typedef int fd_t;
static fd_t clockd_cleanup_fd;
static uint8_t clockd_flags;
void signal_cb(int sig)
{
	(void)sig;
	close(clockd_cleanup_fd);
}
static void check(const bool fail_condition, const char * error_message)
{
	if (fail_condition) {
		perror(error_message);
		signal_cb(0);
		exit(1);
	}
}
void check_fork(void)
{
	if ((clockd_flags & FLAG_FORK) // in forking mode
		&& (fork() != 0)) // fork the child
		exit(0); // quit the parent
}
// simple RFC 868 client/server
static int get_socket(void)
{
	fd_t fd = socket(AF_INET, SOCK_STREAM, 0);
	check(fd < 0, "socket()");
	clockd_cleanup_fd = fd;
	signal(SIGINT, &signal_cb);
	signal(SIGKILL, &signal_cb);
	signal(SIGTERM, &signal_cb);
	return fd;
}
static time_t get_unix_time(const fd_t fd)
{
	int32_t word;
	uint8_t * bytes = (uint8_t *)&word;
	check(read(fd, bytes, 4) < 0, "read()");
	time_t host_order = ntohl(word);
	printf("received %li\n", (long int)host_order);
	// Must operate on 32 bit signed integer per RFC 868.
	return ((int32_t)host_order) - CLOCKDADJ;
}
static void set_unix_time(time_t t)
{
	puts("setting time...");
	struct timeval tv = {t, 0};
	check(settimeofday(&tv, NULL) < 0, "settimeofday()");
}
static void print_time(const char * prefix, const time_t t)
{
	char buf[26];
	printf("%s time:  %s", prefix, ctime_r(&t, buf));
}
__attribute__((noreturn))
static int client(const char * addr)
{
	const fd_t fd = get_socket();
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(37);
	struct hostent * e = gethostbyname(addr);
	check(!e, "gethostbynname()");
	struct in_addr ** alist = (struct in_addr **)e->h_addr_list;
	printf("resolved %s as %s\n", addr, inet_ntoa(*alist[0]));
	check(inet_pton(AF_INET, inet_ntoa(*alist[0]), &sa.sin_addr) < 1,
		"inet_pton()");
	check(connect(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0,
		"connect()");
	time_t t = get_unix_time(fd);
	print_time("local", time(NULL));
	print_time("remote", t);
	if (clockd_flags & FLAG_SET_TIME)
		set_unix_time(t);
	exit(0);
}
static void print_868_time(const fd_t fd)
{
	// ADJ converts the time to be relative to 01 JAN 1900,
	// rather than 01 JAN 1970.
	enum {SZ = 20};
	int32_t t = time(NULL) + CLOCKDADJ;
	t = htonl(t);
	printf("sending %li\n", (long int)ntohl(t));
	uint8_t * bytes = (uint8_t *)&t;
	write(fd, bytes, 4);
}
__attribute__((noreturn))
static int server()
{
	check_fork();
	puts("clockd server starting");
	const fd_t fd = get_socket();
	{ // reuse socket if the kernel still has it
		int optval;
		check(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			&optval, sizeof(int)) < 0, "setsockopt()");
	}
	struct sockaddr_in a = {.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY), .sin_port = htons(37)};
	check(bind(fd, (struct sockaddr *)&a, sizeof(struct sockaddr)) < 0,
		"bind()");
	// accept up to 16 pending connections
	check(listen(fd, 16) < 0, "listen()");
	for (;;) {
		struct sockaddr c_adr;
		socklen_t len;
		int c_fd = accept(fd, &c_adr, &len);
		check(c_fd < 0, "accept()");
		print_868_time(c_fd);
		close(c_fd);
	}
	close(fd);
	exit(0);
}
int main(int argc, char ** argv)
{
	char opt, opts[] = "fSc:s";
	while ((opt = getopt(argc, argv, opts)) != -1) {
		switch(opt) {
		case 'c': // optarg is the ip address
			exit(client(optarg));
		case 'f': // enable forking
			clockd_flags |= FLAG_FORK;
			break;
		case 'S': // set time with client
			clockd_flags |= FLAG_SET_TIME;
			break;
		case 's': // run in server mode
			server();
		default: // display usage
			printf("%s -[%s]\n", argv[0], opts);
			exit(1);
		}
	}
	return 0;
}
