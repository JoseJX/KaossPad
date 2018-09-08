#include "P3A.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool big_endian = false;
#define	SWAP32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) >> 24) & 0xff))

// Reads BLOCK_SIZE bytes from the file
static bool P3A_Read_Block(FILE *f, uint8_t *block) {
	size_t result;
	result = fread(block, 1, BLOCK_SIZE, f);	
	if (result != BLOCK_SIZE) {
		printf("Unable to read the block!\n");
		fclose(f);
		return false;
	}
	return true;
}

// Reads a chunk from the file
static bool P3A_Read_Chunk(FILE *f, P3A_Chunk_t *ch) {
	uint8_t block[BLOCK_SIZE];
	uint32_t i;
	uint8_t *data_offset;

	// Get the first part with the header
	P3A_Read_Block(f, block);

	// Cast the block as a chunk header
	P3A_Chunk_Header_t *head = (P3A_Chunk_Header_t *)block;

	// Copy in the chunk parameters
	memcpy(ch->header.tag, head->tag, 4);
	memcpy(&(ch->header.size), &(head->size), 4);

	// Swap if needed
	if (!big_endian)
		ch->header.size = SWAP32(ch->header.size);

	// Handle the case where the data is sized 0
	if (ch->header.size == 0) {
		ch->data = NULL;
	} else {
		// Allocate the data segment
		ch->data = (uint8_t *)malloc(sizeof(uint8_t) * ch->header.size);
		if (ch->data == NULL) {
			printf("Unable to allocate %d bytes!\n", ch->header.size);
			exit(-1);
		}
		data_offset = ch->data;

		// Copy in the data we already read in
		if (ch->header.size > BLOCK_SIZE) {
			memcpy(data_offset, block + sizeof(P3A_Chunk_Header_t), BLOCK_SIZE - sizeof(P3A_Chunk_Header_t));
			data_offset += (BLOCK_SIZE - sizeof(P3A_Chunk_Header_t));
		} else {
			memcpy(data_offset, block + sizeof(P3A_Chunk_Header_t), ch->header.size);
			data_offset += ch->header.size;
		}

		// Find out how many more blocks to read
		int32_t remaining_bytes = ch->header.size - (BLOCK_SIZE - sizeof(P3A_Chunk_Header_t));
		if (remaining_bytes < 0)
			remaining_bytes = 0;

		int32_t remaining_blocks = (remaining_bytes / BLOCK_SIZE) + (((remaining_bytes % BLOCK_SIZE) > 0) ? 1 : 0);

		for (i=0; i<remaining_blocks; i++) {
			P3A_Read_Block(f, block);

			if (remaining_bytes >= BLOCK_SIZE) {
				memcpy(data_offset, block, BLOCK_SIZE);
				data_offset += BLOCK_SIZE;
				remaining_bytes -= BLOCK_SIZE;
			} else {
				memcpy(data_offset, block, remaining_bytes);
				data_offset += remaining_bytes;
				remaining_bytes = 0;
			}
		}
	}
	return true;
}

// Data appears to be aligned by sector (BLOCK_SIZE bytes)
bool P3A_Parse(const char *fn, P3A_t *p) {
	FILE *f;
	P3A_Header_t *ph;
	uint32_t i;
	uint8_t block[BLOCK_SIZE];

	// Check endianness
	uint8_t swap[4] = { 0x01, 0x23, 0x45, 0x67 };
	uint32_t *swap32 = (uint32_t *)swap;
	if (*swap32 == 0x01234567)
		big_endian = true;

	f = fopen(fn, "rb");
	if (!f) {
		printf("Unable to open the file: %s\n", fn);
		return false;
	}

	// Read in the header
	P3A_Read_Block(f, block);
	ph = (P3A_Header_t *)block;

	// Check the magic
	if (strncmp(ph->magic, "KP3F", 4)) {
		printf("Unknown file type, is this file from a KAOS Pad?\n");
		fclose(f);
		return false;
	}

	// Print out the number of programs and samples
	printf("Globs: %d Programs: %d Samples: %d\n", ph->glob_count, ph->program_count, ph->sample_count);
	p->glob_count = ph->glob_count;
	p->program_count = ph->program_count;
	p->sample_count = ph->sample_count;

	// Read in the globs	
	p->globs = (P3A_Chunk_t *)malloc(sizeof(P3A_Chunk_t) * p->glob_count);
	for (i=0; i<ph->glob_count; i++) {
		P3A_Read_Chunk(f, &(p->globs[i]));
	}

	// Read in the programs
	p->programs = (P3A_Chunk_t *)malloc(sizeof(P3A_Chunk_t) * p->program_count);
	for (i=0; i<ph->program_count; i++) {
		P3A_Read_Chunk(f, &(p->programs[i]));
	}

	// Read in the samples
	p->samples = (P3A_Chunk_t *)malloc(sizeof(P3A_Chunk_t) * p->sample_count);
	for (i=0; i<ph->sample_count; i++) {
		P3A_Read_Chunk(f, &(p->samples[i]));
	}

	fclose(f);
	return true;
}

// Write a sample to disk
void P3A_Write_Sample(P3A_t *p, uint32_t sample, char *fn) {
	uint16_t *ptr, d;
	uint32_t i;
	FILE *f = fopen(fn, "wb");
	size_t result;

	printf("Writing Sample %d to %s\n", sample, fn);

	// Set up the header
	uint32_t wave_hdr[11];
	// "RIFX" header
	memcpy(&wave_hdr[0], "RIFF", 4);
	// Size of the file in bytes
	wave_hdr[1] = p->samples[sample].header.size + 36;	
	// Format "WAVE"
	memcpy(&wave_hdr[2], "WAVE", 4);
	// Sub Chunk 1 ID
	memcpy(&wave_hdr[3], "fmt ", 4);
	// Sub Chunk 1 Size, always 16 for PCM data
	wave_hdr[4] = 16;
	// Audio Format
	ptr = (uint16_t *)&(wave_hdr[5]);
	ptr[0] = 1;	// PCM
	ptr[1] = 2;	// Stereo
	// Sample Rate
	wave_hdr[6] = 48000;
	// Byte Rate 
	wave_hdr[7] = 48000 * 2 * 2; // Sample Rate * Number of channels * (Bits per sample / 8)
	// Block Align
	ptr = (uint16_t *)&(wave_hdr[8]);
	ptr[0] = 2 * 2; // Number of channels * (Bits per sample / 8)
	ptr[1] = 16;	// Bits per sample
	// The data subchunk
	memcpy(&wave_hdr[9], "data", 4);
	// The size of the subchunk
	wave_hdr[10] = p->samples[sample].header.size;

	// Write the header
	result = fwrite(wave_hdr, sizeof(uint32_t), 11, f);
	if (result != 11) {
		printf("Unable to write the RIFF header!\n");
		exit(-1);
	}

	// Write the data, we have to swap to LE
	ptr = (uint16_t *)p->samples[sample].data;
	for (i=0; i<((p->samples[sample].header.size)/2); i++) {
		d = ptr[i];
		d = ((d >> 8) & 0xFF) | ((d << 8) & 0xFF00);
		result = fwrite(&d, sizeof(uint16_t), 1, f);
		if (result != 1) {
			printf("Unable to write to the file %s!\n", fn);
			exit(-1);
		}
	}

	fclose(f);
}
