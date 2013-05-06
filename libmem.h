
#include "stddef.h"

typedef struct {
	size_t node;
	char base[0];
} libmem_Segment;

typedef struct {
	size_t sibling	: 30;
	size_t parent	: 30;
	size_t prevous	: 30;
	size_t next		: 30;
	size_t level	: 7;
	size_t free		: 1;
	libmem_Segment* segment;
} libmem_Node;

typedef struct {
	size_t minlevel;
	size_t maxlevel;
	size_t nodefree;
	size_t nodesize;
	char* base;
	size_t* free;
	libmem_Node* node;
} libmem;


libmem* libmem_new(size_t minlevel, size_t maxlevel);
void libmem_delete(libmem* self);
void* libmem_alloc(libmem* self, size_t size);
void* libmem_realloc(libmem* self, void* ptr, size_t size);
int libmem_free(libmem* self, void* ptr);