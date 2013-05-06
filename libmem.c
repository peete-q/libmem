
#include "assert.h"
#include "libmem.h"

#ifndef NULL
#define NULL 0
#endif
#define INVALID -1

static void libmem_resizenode(libmem* self, size_t size) {
	size_t i;
	libmem_Node* node;
	
	if (self->node)
		node = (libmem_Node*) realloc(self->node, sizeof(libmem_Node) * size);
	else
		node = (libmem_Node*) malloc(sizeof(libmem_Node) * size);
	
	if (node)
	{
		self->node = node;
		for (i = self->nodesize; i < size - 1; ++i)
			self->node[i].next = i + 1;
		self->node[size - 1].next = INVALID;
		self->nodefree = self->nodesize;
		self->nodesize = size;
	}
}

static size_t libmem_allocnode(libmem* self) {
	size_t i;
	if (self->nodefree == INVALID)
		libmem_resizenode(self, self->nodesize << 1);
	i = self->nodefree;
	self->nodefree = self->node[i].next;
	return i;
}

static void libmem_freenode(libmem* self, size_t i) {
	if (self->node[i].next != INVALID)
		self->node[self->node[i].next].prevous = self->node[i].prevous;
	if (self->node[i].prevous != INVALID)
		self->node[self->node[i].prevous].next = self->node[i].next;
		
	self->node[i].next = self->nodefree;
	self->nodefree = i;
}

static size_t libmem_splitnode(libmem* self, size_t parent) {
	size_t i, j, k;
	libmem_Node* node = &self->node[parent];
	
	i = libmem_allocnode(self);
	j = libmem_allocnode(self);
	if (i == INVALID || j == INVALID)
		return INVALID;
		
	self->node[i].parent = parent;
	self->node[i].sibling = j;
	self->node[i].level = node->level - 1;
	self->node[i].segment = node->segment;
	self->node[i].segment->node = i;
	
	self->node[j].parent = parent;
	self->node[j].sibling = i;
	self->node[j].level = node->level - 1;
	self->node[j].segment = (libmem_Segment*) ((char*)node->segment + (1 << (node->level - 1)));
	self->node[j].segment->node = j;
	
	k = self->free[node->level - self->minlevel - 1];
	self->node[j].prevous = INVALID;
	self->node[j].next = k;
	if (k != INVALID)
		self->node[k].prevous = j;
	self->free[node->level - self->minlevel - 1] = j;
	return i;
}

static void libmem_combinnode(libmem* self, size_t child) {
	size_t i;
	if (self->node[child].sibling != INVALID && self->node[self->node[child].sibling].free)
	{
		libmem_combinnode(self, self->node[child].parent);
		libmem_freenode(self, child);
		libmem_freenode(self, self->node[child].sibling);
	}
	else
	{
		i = self->free[self->node[child].level - self->minlevel];
		self->node[child].free = 1;
		self->node[child].prevous = INVALID;
		self->node[child].next = i;
		if (i != INVALID)
			self->node[i].prevous = child;
		self->free[self->node[child].level - self->minlevel] = child;
	}
}

static void* libmem_allocbylevel(libmem* self, size_t level) {
	libmem_Node* node;
	size_t i, j;
	
	node = NULL;
	i = level - self->minlevel;
	if (self->free[i] != INVALID)
	{
		node = &self->node[self->free[i]];
		self->free[i] = node->next;
	}
	else for (++i; i < self->maxlevel - self->minlevel; ++i)
	{
		if (self->free[i] != INVALID)
		{
			j = self->free[i];
			self->free[i] = self->node[j].next;
			for (; i > level - self->minlevel; --i)
			{
				j = libmem_splitnode(self, j);
				if (j == INVALID) return NULL;
			}
			node = &self->node[j];
			break;
		}
	}
	return node ? node->segment->base : NULL;
}

libmem* libmem_new(size_t minlevel, size_t maxlevel) {
	libmem* self;
	size_t i;
	assert(minlevel < maxlevel);
	
	self = (libmem*) malloc(sizeof(libmem));
	self->base = (char*) malloc(sizeof(char) * (1 << maxlevel));
	self->free = (size_t*) malloc(sizeof(size_t) * (maxlevel - minlevel));
	self->minlevel = minlevel;
	self->maxlevel = maxlevel;
	
	self->nodesize = 0;
	self->node = NULL;
	libmem_resizenode(self, 64);
	
	for (i = 0; i < maxlevel - minlevel - 1; ++i)
		self->free[i] = INVALID;
	self->free[i] = libmem_allocnode(self);
	i = self->free[i];
	self->node[i].sibling = INVALID;
	self->node[i].parent = INVALID;
	self->node[i].prevous = INVALID;
	self->node[i].next = INVALID;
	
	self->node[i].segment = (libmem_Segment*) self->base;
	self->node[i].segment->node = i;
	self->node[i].level = maxlevel;
	return self;
}

void libmem_delete(libmem* self) {
	free(self->node);
	free(self->base);
	free(self->free);
	free(self);
}

void* libmem_alloc(libmem* self, size_t size) {
	size_t i, realsize = size + sizeof(libmem_Segment);
	assert(realsize <= (1 << self->maxlevel));
	
	for (i = self->minlevel; i < self->maxlevel; ++i)
		if (realsize <= (1 << i)) break;
	return libmem_allocbylevel(self, i);
}

void* libmem_realloc(libmem* self, void* ptr, size_t size) {
	char* base;
	libmem_Segment* segment;
	base = (char*) ptr;
	if (self->base <= base && base < self->base + (1 << self->maxlevel))
	{
		segment = (libmem_Segment*)(base - sizeof(libmem_Segment));
		if ((1 << self->node[segment->node].level) - sizeof(libmem_Segment) >= size)
			return ptr;
			
		libmem_free(self, ptr);
		return libmem_alloc(self, size);
	}
	return NULL;
}

int libmem_free(libmem* self, void* ptr) {
	char* base;
	libmem_Segment* segment;
	base = (char*) ptr;
	if (self->base <= base && base < self->base + (1 << self->maxlevel))
	{
		segment = (libmem_Segment*)(base - sizeof(libmem_Segment));
		libmem_combinnode(self, segment->node);
		return 1;
	}
	return 0;
}

