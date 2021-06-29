#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <lz4.h>

#define EXE_SIZE 2142208

#define MIN_WIDTH 1024
#define MAX_WIDTH 7680
#define MIN_HEIGHT 720
#define MAX_HEIGHT 4320

uint32_t widthOffsets[] =
{
	0xcfa0a,
	0xcfda3
};

uint32_t heightOffsets[] =
{
	0xcfa0f,
	0xcfdad
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

uint8_t exeFile[EXE_SIZE];

/* Patch a little-endian 32bit value in. */
void PatchU32(uint32_t offset, uint32_t value)
{
	exeFile[offset] = value & 0xFF;
	exeFile[offset+1] = (value >> 8) & 0xFF;
	exeFile[offset+2] = (value >> 16) & 0xFF;
	exeFile[offset+3] = (value >> 24) & 0xFF;
}

void PatchResolution(int width, int height)
{
	for (int i = 0; i < ARRAY_SIZE(widthOffsets); ++i)
	{
		PatchU32(widthOffsets[i], width);
	}
	for (int i = 0; i < ARRAY_SIZE(heightOffsets); ++i)
	{
		PatchU32(heightOffsets[i], height);
	}
}

bool ValidateResolution(int width, int height)
{
	bool valid = true;
	if (width & 3)
	{
		fprintf(stderr, "The width %d is not a multiple of 4.\n", width);
		fprintf(stderr, "Pharaoh requires the resolution's width to be divisible by 4.\n");
		valid = false;
	}

	if (width < MIN_WIDTH)
	{
		fprintf(stderr, "The width %d is less than %d.\n", width, MIN_WIDTH);
		fprintf(stderr, "The selected width must be at least %d pixels.\n", MIN_WIDTH);
		valid = false;
	}

	if (width > MAX_WIDTH)
	{
		fprintf(stderr, "The width %d is greater than %d.\n", width, MAX_WIDTH);
		fprintf(stderr, "The selected width must be no greater than %d pixels.\n", MAX_WIDTH);
		valid = false;
	}

	if (height & 3)
	{
		fprintf(stderr, "The height %d is not a multiple of 4.\n", height);
		fprintf(stderr, "Pharaoh requres the resolution's height to be divisible by 4.\n");
		valid = false;
	}

	if (height < MIN_HEIGHT)
	{
		fprintf(stderr, "The height %d is less than %d.\n", height, MIN_HEIGHT);
		fprintf(stderr, "The selected height must be at least %d pixels.\n", MIN_HEIGHT);
		valid = false;
	}

	if (height > MAX_HEIGHT)
	{
		fprintf(stderr, "The height %d is greater than %d.\n", height, MAX_HEIGHT);
		fprintf(stderr, "The selected height must be no greater than %d pixels.\n", MAX_HEIGHT);
		valid = false;
	}

	return valid;
}

int main(int argc, char **argv)
{
	if (argc < 5)
	{
		printf("%s: Run Pharaoh+Cleopatra at different resolutions\n", argv[0]);
		printf("\n");
		printf("Usage:\n");
		printf("\t%s [EXE_RESOURCE] [output] [width] [height]\n", argv[0]);
		printf("\t[EXE_RESOURCE]: the LZ4 compressed patch executable\n");
		printf("\t[output]: The .exe file to write\n");
		printf("\t[width]: The width of the resolution to use.\n");
		printf("\t[height]: The height of the resolution to use.\n");
		return 1;
	}

	const char *inName = argv[1];
	const char *outName = argv[2];
	int width = atoi(argv[3]);
	int height = atoi(argv[4]);

	FILE *inFile = fopen(inName, "rb");
	if (!inFile)
	{
		fprintf(stderr, "Couldn't open EXE_RESOURCE \"%s\"\n", inName);
		fprintf(stderr, "You can extract it from PharaohResizer.exe with:\n");
		fprintf(stderr, "\twrestool PharaohResizer.exe -x --type='EXECUTABLE' --name=101\n");
		return 2;
	}

	FILE *outFile = fopen(outName, "wb");
	if (!outFile)
	{
		fprintf(stderr, "Couldn't open output file \"%s\"\n", outName);
		fprintf(stderr, "Check that the path is correct and is writable.\n");
		fclose(inFile);
		return 3;
	}

	fseek(inFile, 0, SEEK_END);
	size_t compSize = ftell(inFile);
	fseek(inFile, 0, SEEK_SET);
	uint8_t *compData = malloc(compSize);
	if (!compData)
	{
		fprintf(stderr, "Not enough memory to load compressed file \"%s\"\n", inName);
		fprintf(stderr, "Is it mysteriously enormous? It appears to be %lu MiB.\n", compSize / (1024 * 1024));
		fclose(inFile);
		fclose(outFile);
		return 4;
	}
	if (fread(compData, compSize, 1, inFile) != 1)
	{
		fprintf(stderr, "Couldn't read input file \"%s\"\n", inName);
		fclose(inFile);
		fclose(outFile);
		return 5;
	}
	fclose(inFile);

	int retVal = LZ4_decompress_safe(compData, exeFile, compSize, EXE_SIZE);

	if (retVal < 1)
	{
		fprintf(stderr, "Couldn't decompress input file \"%s\"\n", inName);
		fprintf(stderr, "Check that it's the correct resource extracted from PharaohResizer.exe\n");
		return 6;
	}

	if (!ValidateResolution(width, height))
	{
		/* ValidateResolution prints its own errors. */
		return 7;
	}

	/* Patch the user's width and height in. */
	PatchResolution(width, height);

	if (fwrite(exeFile, EXE_SIZE, 1, outFile) != 1)
	{
		fprintf(stderr, "Couldn't write output executable \"%s\"\n", outName);
		fprintf(stderr, "Have you run out of disk space or something?\n");
		fclose(outFile);
		return 8;
	}
	fclose(outFile);
	return 0;
}
