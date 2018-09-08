#include <stdio.h>
#include <stdlib.h>

#include "P3A.h"

int main(int argc, char **argv) {
	P3A_t p;

	if (argc == 1) {
		printf("Usage:\n\t%s <P3A File>\n", argv[0]);
		exit (-1);
	}

	P3A_Parse(argv[1], &p);
	P3A_Write_Sample(&p, 0, "sample0.wav");
	P3A_Write_Sample(&p, 1, "sample1.wav");
	P3A_Write_Sample(&p, 2, "sample2.wav");
	P3A_Write_Sample(&p, 3, "sample3.wav");

	exit(0);
}
