#include "libmem.h"

#include <assert.h>

#define INVALID -1

struct libmem_Segment {
	size_t index;
	char base[0];
};

struct libmem_Node {
	size_t sibling;
	size_t parent;
	size_t prevous;
	size_t next;
	size_t level	: 7;
	size_t free		: 1;
	struct libmem_Segment* segment;
};

struct libmem {
	size_t minlevel;
	size_t maxlevel;
	size_t nodefree;
	size_t nodesize;
	char* base;
	size_t* free;
	struct libmem_Node* node;
};

static size_t libmem_resizenode(struct libmem* self, size_t size) {
	size_t i;
	struct libmem_Node* node;
	
	if (self->node)
		node = (struct libmem_Node*) realloc(self->node, sizeof(struct libmem_Node) * size);
	else
		node = (struct libmem_Node*) malloc(sizeof(struct libmem_Node) * size);
	
	if (node)
	{
		self->node = node;
		for (i = self->nodesize; i < size - 1; ++i)
			self->node[i].next = i + 1;
		self->node[size - 1].next = INVALID;
		self->nodefree = self->nodesize;
		self->nodesize = size;
	}
	return self->nodefree;
}

static size_t libmem_allocnode(struct libmem* self) {
	size_t i;
	if (self->nodefree == INVALID)
		libmem_resizenode(self, self->nodesize << 1);
	if (self->nodefree == INVALID)
		return INVALID;
	i = self->nodefree;
	self->nodefree = self->node[i].next;
	
	self->node[i].sibling = INVALID;
	self->node[i].parent = INVALID;
	self->node[i].prevous = INVALID;
	self->node[i].next = INVALID;
	return i;
}

static void libmem_freenode(struct libmem* self, size_t i) {
	if (self->node[i].next != INVALID)
		self->node[self->node[i].next].prevous = self->node[i].prevous;
	if (self->node[i].prevous != INVALID)
		self->node[self->node[i].prevous].next = self->node[i].next;
		
	self->node[i].next = self->nodefree;
	self->nodefree = i;
}

static size_t libmem_splitnode(struct libmem* self, size_t parent) {
	size_t i, j, k;
	struct libmem_Node* node;
	
	i = libmem_allocnode(self);
	j = libmem_allocnode(self);
	if (i == INVALID || j == INVALID)
		return INVALID;
		
	node = &self->node[parent];
	node->free = 0;
		
	self->node[i].parent = parent;
	self->node[i].sibling = j;
	self->node[i].level = node->level - 1;
	self->node[i].segment = node->segment;
	self->node[i].segment->index = i;
	self->node[i].free = 0;
	
	self->node[j].parent = parent;
	self->node[j].sibling = i;
	self->node[j].level = node->level - 1;
	self->node[j].segment = (struct libmem_Segment*) ((char*)node->segment + (1 << (node->level - 1)));
	self->node[j].segment->index = j;
	
	k = self->free[node->level - self->minlevel - 1];
	self->node[j].prevous = INVALID;
	self->node[j].next = k;
	self->node[j].free = 1;
	if (k != INVALID)
		self->node[k].prevous = j;
	self->free[node->level - self->minlevel - 1] = j;
	return i;
}

static void libmem_combinnode(struct libmem* self, size_t child) {
	size_t i;
	if (self->node[child].sibling != INVALID && self->node[self->node[child].sibling].free)
	{
		libmem_combinnode(self, self->node[child].parent);
		libmem_freenode(self, child);
		libmem_freenode(self, self->node[child].sibling);
	}
	else
	{
		i = self->free[self->node[child].level - self->minlevel - 1];
		self->node[child].free = 1;
		self->node[child].prevous = INVALID;
		self->node[child].next = i;
		if (i != INVALID)
			self->node[i].prevous = child;
		self->free[self->node[child].level - self->minlevel - 1] = child;
	}
}

static void* libmem_allocbylevel(struct libmem* self, size_t level) {
	struct libmem_Node* node;
	size_t i, j;
	
	node = NULL;
	i = level - self->minlevel - 1;
	if (self->free[i] != INVALID)
	{
		node = &self->node[self->free[i]];
		node->free = 0;
		self->free[i] = node->next;
	}
	else for (++i; i < self->maxlevel - self->minlevel - 1; ++i)
	{
		if (self->free[i] != INVALID)
		{
			j = self->free[i];
			self->free[i] = self->node[j].next;
			for (; i > level - self->minlevel - 1; --i)
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

struct libmem* libmem_new(size_t minlevel, size_t maxlevel) {
	struct libmem* self;
	size_t i;
	assert(minlevel < maxlevel);
	
	self = (struct libmem*) malloc(sizeof(struct libmem));
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
	self->node[i].free = 1;
	
	self->node[i].segment = (struct libmem_Segment*) self->base;
	self->node[i].segment->index = i;
	self->node[i].level = maxlevel;
	return self;
}

void libmem_delete(struct libmem* self) {
	free(self->node);
	free(self->base);
	free(self->free);
	free(self);
}

void* libmem_alloc(struct libmem* self, size_t size) {
	size_t i, min, max, realsize;
	realsize = size + sizeof(struct libmem_Segment);
	assert(realsize <= (1 << self->maxlevel));
	
	min = self->minlevel;
	max = self->maxlevel;
	
	while (max - min > 1)
	{
		i = (min + max) >> 1;
		if (realsize > (1 << i))
			min = i;
		else
			max = i;
	}
	return libmem_allocbylevel(self, i);
}

void* libmem_realloc(struct libmem* self, void* ptr, size_t size) {
	char* base;
	struct libmem_Segment* segment;
	base = (char*) ptr;
	if (self->base <= base && base < self->base + (1 << self->maxlevel))
	{
		segment = (struct libmem_Segment*)(base - sizeof(struct libmem_Segment));
		if ((1 << self->node[segment->index].level) - sizeof(struct libmem_Segment) >= size)
			return ptr;
			
		libmem_free(self, ptr);
		return libmem_alloc(self, size);
	}
	return NULL;
}

int libmem_free(struct libmem* self, void* ptr) {
	char* base;
	struct libmem_Segment* segment;
	base = (char*) ptr;
	if (self->base <= base && base < self->base + (1 << self->maxlevel))
	{
		segment = (struct libmem_Segment*)(base - sizeof(struct libmem_Segment));
		libmem_combinnode(self, segment->index);
		return 1;
	}
	return 0;
}
