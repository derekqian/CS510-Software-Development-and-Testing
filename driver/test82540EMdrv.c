/*
 * Derek Qian
 * 02/22/2013
 * CS 510 - Software Development and Testing
 *
 * Project 4: appliction to test 82540EM driver.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


int main (int argc, char * argv[]) {
	int i, fd, rv;
	char * devname = "/dev/82540EMdrv";
	unsigned int buf[2];
	ssize_t hasread, haswritten;
	int count = 1;
	char * str = "Hello, 82540EMdrv!";

	switch (argc) 
	{
	case 3:
		str = argv[2];
	case 2:
		count = atoi(argv[1]);
	case 1:
		break;
	default:
		printf("Usage: test82540EMdrv [times to read] [string to write]\n");
		exit (-1);
	}

	fd = open (devname, O_RDWR);
	if (fd < 0) {
		printf("test82540EMdrv: open %s failed.\n", devname);
		exit (-1);
	} else {
		printf("test82540EMdrv: %s opened successfully!\n", devname);
	}

	for (i=0; i<count; i++) {
		hasread = read (fd, buf, sizeof(unsigned int)*2);
		if (hasread != sizeof(unsigned int)*2) {
			printf("test82540EMdrv: Read %s failed.\n", devname);
			exit (-1);
		} else {
			printf("test82540EMdrv: read %s successfully (%d - 0x%08x)!\n", devname, buf[0], buf[1]);
		}
	}

	haswritten = write (fd, str, strlen(str)+1);
	if (haswritten != strlen(str)+1) {
		printf("test82540EMdrv: Write \"%s\" to %s failed.\n", str, devname);
		exit (-1);
	} else {
		printf("test82540EMdrv: write \"%s\" to %s successfully!\n", str, devname);
	}

	rv = close (fd);
	if (rv < 0) {
		printf("test82540EMdrv: %s close failed.\n", devname);
		exit (-1);
	} else {
		printf("test82540EMdrv: %s closed successfully!\n", devname);
	}
}
