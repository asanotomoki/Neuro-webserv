#include <stdio.h>

int main(int argc, char *argv[]) {
	printf("Content-Type: text/html\r\n\r\n");
    printf("<html><head><title>Hello World</title></head>");
	printf("<body><h1>Hello World</h1>");
	if (argc > 1) {
		printf("%s", argv[1]);
	}
	printf("</body></html>");

    return 0;
}

