#ifndef __PARSE_P3A_H__
#define __PARSE_P3A_H__

#include <stdint.h>
#include <stdbool.h>

#define BLOCK_SIZE	512

// NOTE: The data here is big endian!

typedef struct {
	char magic[4];
	uint8_t glob_count;
	uint8_t program_count;
	uint8_t sample_count;
	uint8_t unk_values[5];
} P3A_Header_t;

typedef struct {
	char tag[4];
	uint32_t size;
	uint32_t unk[2];
} P3A_Chunk_Header_t;

typedef struct {
	P3A_Chunk_Header_t header;
	uint8_t *data;
} P3A_Chunk_t;

// Container for the data we've read in
typedef struct {
	uint8_t glob_count;
	uint8_t program_count;
	uint8_t sample_count;
	uint8_t align_unused;

	P3A_Chunk_t *globs;
	P3A_Chunk_t *programs;
	P3A_Chunk_t *samples;
} P3A_t;

// The parsing function
bool P3A_Parse(const char *fn, P3A_t *p);

// Write a sample to disk
void P3A_Write_Sample(P3A_t *p, uint32_t sample, char *fn);

#endif
