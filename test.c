
#include "stdlib.h"
#include "stdint.h"
#include "time.h"
#include "libmem.h"

char* getarg(int argc, char** argv, char* name, char* def) {
	int i, len = strlen(name);
	for (i = 0; i < argc; ++i)
		if (strncmp(argv[i], name, len) == 0)
			return argv[i] + len;
	return def;
}

void main(int argc, char** argv) {
	int i, j, k, s, size, count, usecase, min, max;
	void** ptr;
	
	size = atoi(getarg(argc, argv, "-s=", "256"));
	count = atoi(getarg(argc, argv, "-c=", "100000"));
	usecase = pow(2, atoi(getarg(argc, argv, "-u=", "14")));
	min = atoi(getarg(argc, argv, "-min=", "8"));
	max = atoi(getarg(argc, argv, "-max=", "28"));
	
	ptr = (void**) malloc(sizeof(void*) * size);
	s = rand();
	libmem* self = libmem_new(min, max);
	
	clock_t c = clock();
	srand(s);
	for (k = 0; k < count; ++k)
	{
		i = rand() % size;
		for (j = 0; j < i; ++j)
			ptr[j] = libmem_alloc(self, (uint64_t)rand() * usecase / RAND_MAX);
		for (j = 0; j < i; ++j)
			libmem_free(self, ptr[j]);
	}
	printf("libmem cost %d\n", (clock() - c));
	
	c = clock();
	srand(s);
	for (k = 0; k < count; ++k)
	{
		i = rand() % size;
		for (j = 0; j < i; ++j)
			ptr[j] = malloc((uint64_t)rand() * usecase / RAND_MAX);
		for (j = 0; j < i; ++j)
			free(ptr[j]);
	}
	printf("malloc cost %d\n", (clock() - c));
	
	libmem_delete(self);
	free(ptr);
}
