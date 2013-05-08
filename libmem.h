
#ifndef __LIBMEM_H__
#define __LIBMEM_H__

#include <stddef.h>

struct libmem;

struct libmem* libmem_new(size_t minlevel, size_t maxlevel);
void libmem_delete(struct libmem* self);
void* libmem_alloc(struct libmem* self, size_t size);
void* libmem_realloc(struct libmem* self, void* ptr, size_t size);
int libmem_free(struct libmem* self, void* ptr);

#endif
