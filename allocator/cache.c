#include "alloc.h"
#include "xerror.h"
#include <stdlib.h>

static struct lru_node *_lru_find(ALLOC * a, handle_t h);
static void _list_add(ALLOC * a, struct lru_node *x);
static void _list_remove(ALLOC * a, struct lru_node *x);
static void _lru_mv2head(ALLOC * a, struct lru_node *x);
static struct lru_node *_newnode();
static void _freenode(ALLOC * a, struct lru_node *x);
static void _hash_add(ALLOC * a, struct lru_node *x);
static void _hash_del(ALLOC * a, handle_t h);
static void _cache_shrink(ALLOC * a, size_t to);

struct lru_node *_lru_find(ALLOC * a, handle_t h)
{
	int idx = h % LRU_NSLOT;
	struct lru_node *x;

	for (x = a->lru[idx]; x != NULL; x = x->hash_next)
		if (x->self == h)
			return x;
	return NULL;
}

void _list_add(ALLOC * a, struct lru_node *x)
{
	x->next = a->lru_head;
	if (x->next != NULL)
		x->next->prev = x;
	a->lru_head = x;
	if (a->lru_tail == NULL)
		a->lru_tail = x;
}

void _list_remove(ALLOC * a, struct lru_node *x)
{
	if (x->prev != NULL)
		x->prev->next = x->next;
	else
		a->lru_head = x->next;

	if (x->next != NULL)
		x->next->prev = x->prev;
	else
		a->lru_tail = x->prev;
}

void _lru_mv2head(ALLOC * a, struct lru_node *x)
{
	_list_remove(a, x);
	_list_add(a, x);
}

struct lru_node *_newnode()
{
	return calloc(1, sizeof(struct lru_node));
}

void _freenode(ALLOC * a, struct lru_node *x)
{
	buf_put(a, x->block);
	free(x);
}

void _hash_add(ALLOC * a, struct lru_node *x)
{
	int idx = x->self % LRU_NSLOT;

	x->hash_next = a->lru[idx];
	a->lru[idx] = x;
}

void _hash_del(ALLOC * a, handle_t h)
{
	int idx = h % LRU_NSLOT;
	struct lru_node *x, **pp;

	for (pp = &a->lru[idx]; (x = *pp) != NULL; pp = &(*pp)->hash_next)
		if (x->self == h) {
			*pp = x->hash_next;
			break;
		}
}

void _cache_shrink(ALLOC * a, size_t to)
{
	struct lru_node *x, *prev;
	for (x = a->lru_tail; x != NULL && a->lru_size > to; x = prev) {
		prev = x->prev;
		_hash_del(a, x->self);
		_list_remove(a, x);
		a->lru_size -= x->size;
		_freenode(a, x);
	}
}

int cache_set(ALLOC * a, handle_t h, void *buf, size_t len)
{
	_cache_shrink(a, LRU_SIZE - len);

	struct lru_node *x = _lru_find(a, h);
	if (x == NULL) {
		if ((x = _newnode()) == NULL) {
			xerrno = FATAL_NOMEM;
			return -1;
		}
		a->lru_size += len;
		x->self = h;
		x->block = buf;
		x->size = len;
		_hash_add(a, x);
		_list_add(a, x);
		return 0;
	}
	// found
	buf_put(a, x->block);
	a->lru_size -= x->size;
	a->lru_size += len;
	x->block = buf;
	x->size = len;
	_lru_mv2head(a, x);

	return 0;
}

void *cache_get(ALLOC * a, handle_t h, size_t * len)
{
	struct lru_node *x = _lru_find(a, h);

	if (x == NULL)
		return NULL;
	_lru_mv2head(a, x);
	*len = x->size;
	return x->block;
}

void cache_del(ALLOC * a, handle_t h)
{
	struct lru_node *x = _lru_find(a, h);

	if (x == NULL)
		return;
	a->lru_size -= x->size;
	_list_remove(a, x);
	_hash_del(a, x->self);
}
