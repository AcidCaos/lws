// A light-weight web server. By AcidCaos, 2022.

#define _GNU_SOURCE // Enable use of DT_DIR. DT_DIR is not part of the POSIX but a glibc extension. d_type isn't even mentioned in POSIX's definition of struct dirent.

#include <unistd.h> // system call wrapper functions
#include <stdlib.h> // C standard library
#include <string.h> // String functions
#include <sys/socket.h> // Sockets
#include <arpa/inet.h>  // Internet types and functions
#include <dirent.h> // DIR
#include <sys/stat.h> // Directory stat
#include <fcntl.h> // Open a file
#include <errno.h>

#include <stdarg.h>
#include <stdio.h>

#define COMMAND "lws"
#define DESCRIPTION "A light-weight web server."
#define VERSION "0.1a"
#define BUILD "260223"

#define WELCOME_MESSAGE COMMAND " - " DESCRIPTION " Build " VERSION "." BUILD "\n"

char cwd[256]; // Web server root

char request_buffer[1024];
char response_buffer[4096];
char content[4096];

void usage (char* exec) {
	char buffer[256];
	strcpy(buffer, "Usage: ");
	strcat(buffer, exec);
	strcat(buffer, " PORT\n");
	write(1, buffer, strlen(buffer));
	exit(1);
}

char* http_response (char* status_code, char* reason_phrase, char* content_type, char* body) {
	strcpy(response_buffer, "HTTP/1.0 ");
	strcat(response_buffer, status_code);
	strcat(response_buffer, " ");
	strcat(response_buffer, reason_phrase);
	strcat(response_buffer, "\r\nServer: lws/" VERSION "\r\nContent-Type: ");
	strcat(response_buffer, content_type);
	strcat(response_buffer, "\r\n\r\n");
	strcat(response_buffer, body);
	return response_buffer;
}

char* http_404 () {
	return http_response("404", "Not found", "text/html", "<html><title>404 Not Found</title><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>");
}

char* http_200 (char* content_type, char* body) {
	return http_response("200", "OK", content_type, body);
}

char* http (char* request_buffer) {
	char request_method[16];
	char request_path[256];

	char requested_file_path[512];

	char response_html[1024];

	int index, start;

	// Get HTTP Method
	start = 0;
	index = 0;
	for (; request_buffer[index] != ' '; ++index);
	strncpy(request_method, request_buffer, index);
	request_method[index] = '\0';

	// Get request Path
	start = index + 1;
	index = index + 1;
	for (; request_buffer[index] != ' '; ++index);
	strncpy(request_path, request_buffer + start, index - start);
	request_path[index - start] = '\0';

	// File path
	strcpy(requested_file_path, cwd);
	strcat(requested_file_path, request_path);

	struct stat path_s;
	int ret = stat(requested_file_path, &path_s);

	// Not exists
	if (ret < 0) {
		return http_404();
	}

	// Is a directory
	if S_ISDIR(path_s.st_mode) {
		DIR* pdir = opendir(requested_file_path);
		struct dirent *pdirent;
		strcpy(response_html, "<h1>Directory ");
		strcat(response_html, request_path);
		strcat(response_html, "</h1><br>");
		while ((pdirent = readdir(pdir)) != NULL) {
			// Link url
			strcat(response_html, "<a href=\"");
			strcat(response_html, pdirent->d_name);
			// Link icon
			if (pdirent->d_type == DT_DIR) strcat(response_html, "/\">&#x1F4C1; ");
			else if (pdirent->d_type == DT_LNK) strcat(response_html, "\">&#x1F517; ");
			else strcat(response_html, "\">&#x1F4C4; ");
			// Link text
			strcat(response_html, pdirent->d_name);
			strcat(response_html, "</a><br>");
		}
		return http_200("text/html", response_html);
	}

	// Is a regular file
	if S_ISREG(path_s.st_mode) {
		// content = (char*) malloc(path_s.st_size);
		int fd = open(requested_file_path, O_RDONLY);
		// read(fd, content, path_s.st_size);
		read(fd, content, sizeof(content));
		close(fd);
		return http_200("text/plain", content);
	}

	return http_404();
}

int main (int argc, char* argv[]) {
	char buffer[512];

    int port, cli_size;
	int sock_fd, conn_fd;
	struct sockaddr_in srv_addr, cli_addr;

	// Basic checks
	if (argc != 2) usage(argv[0]);

	// Welcome screen
	write(1, WELCOME_MESSAGE, strlen(WELCOME_MESSAGE));

	// Get port provided
	port = atoi(argv[1]);

	// Save Current Working Directory
	getcwd(cwd, sizeof(cwd));

	// Create socket
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	// Define server address and port
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Bind socket to an IP (single interface, or all) and a PORT
	if (bind(sock_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr))) { perror("bind"); exit(1); }

	// Listen to start receiving connections
	if (listen(sock_fd, 1)) { perror("listen"); exit(1); }

	// Accept a received connection
	cli_size = sizeof(cli_addr);

	while (1) {
		conn_fd = accept(sock_fd, (struct sockaddr*) &cli_addr, (unsigned int*) &cli_size);
		char* ip;

		// Read client's message
		read(conn_fd, request_buffer, sizeof(request_buffer));
		int index = 0;

		// Log request
		ip = inet_ntoa(cli_addr.sin_addr);
		strcpy(buffer, ip);
		strcat(buffer, " -- ");
		for (; request_buffer[index] != '\n'; ++index); // First line of the request
		strncat(buffer, request_buffer, index + 1);
		write(1, buffer, strlen(buffer));

		// HTTP response
		char* res = http(request_buffer);

		// Send response
		write(conn_fd, res, strlen(res));
		// write(1, res, strlen(res) + 1); // Debug response

		// Close connection
		close(conn_fd);
	}
	// Close connection
	close(conn_fd);

	// Close socket
	close(sock_fd);

	return 0;
}
