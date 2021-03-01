#include <stdc.h>

#include "sparse.h"

#define LOAD_FACTOR 80

typedef struct SparseSlot Slot;

void
sparse_init(Sparse *s, int bits)
{
	size_t idx;
	s->bits = bits;
	s->max  = 1 << bits;
	s->load = 0;
	s->slots = malloc(s->max * sizeof *s->slots);
	for (idx = 0; idx < s->max; idx++) {
		s->slots[idx] = (Slot) { 0, -1, NULL };
	}
}

static size_t
indexof(uintmax_t key, int bits)
{
#if UINTMAX_MAX >> 31 == 1
	const int width = 32;
	const uintmax_t cnst = UINT32_C(123456789);
#elif UINTMAX_MAX >> 63 == 1
	const int width = 64;
	const uintmax_t cnst = UINT64_C(1234567891234567891);
#else
#	error "Unsupported uintmax_t size."
#endif
	return (key * cnst) >> (width - bits);
}

static int
search(Sparse *s, uintmax_t key, size_t *idxp, intptr_t *pslp)
{
	size_t idx;
	intptr_t psl;
	Slot *slot;
	
	idx = indexof(key, s->bits);
	psl = 0;
	for (;;) {
		slot = &s->slots[idx];
		if (slot->key == key) {
			*idxp = idx;
			if (pslp) *pslp = psl;
			return 1;
		}
		if (slot->psl < psl) {
			*idxp = idx;
			if (pslp) *pslp = psl;
			return 0;
		}
		idx++;
		psl++;
		idx &= s->max - 1;
	}
}

void *
sparse_get(Sparse *s, uintmax_t key)
{
	size_t idx;

	if (search(s, key, &idx, NULL)) {
		return s->slots[idx].val;
	} else {
		return NULL;
	}
}

static void
insert(Sparse *s, uintmax_t key, void *val)
{
	size_t idx;
	intptr_t psl;
	Slot old;
	int f;
	
	for (;;) {
		f = search(s, key, &idx, &psl);
		old = s->slots[idx];
		s->slots[idx] = (Slot) { key, psl, val };
		if (f) break;
		if (old.psl < 0) {
			s->load++;
			break;
		}
		key = old.key;
		val = old.val;
	}
}

static void
delete(Sparse *s, uintmax_t key)
{
	size_t idx;
	Slot *a, *b;

	if (!search(s, key, &idx, NULL)) {
		return;
	}
	a = &s->slots[idx];
	for (;;) {
		idx++;
		idx &= s->max - 1;
		b = &s->slots[idx];
		if (b->psl <= 0) break;
		*a = *b;
		a = b;
	}
	*a = (Slot) { 0, -1, NULL };
}

static void
resize(Sparse *s, int bits)
{
	size_t idx;
	Sparse n;
	Slot *slot;
	sparse_init(&n, bits);
	for (idx = 0; idx < s->max; idx++) {
		slot = &s->slots[idx];
		if (slot->psl < 0) continue;
		insert(&n, slot->key, slot->val);
	}
	sparse_free(s);
	*s = n;
}

void
sparse_set(Sparse *s, uintmax_t key, void *value)
{
	if (value) {
		if (s->load * 100 > s->max * LOAD_FACTOR) {
			resize(s, s->bits + 1);
		}
		insert(s, key, value);
	} else {
		delete(s, key);
		if (s->bits > 4 && s->load * 100 < (s->max / 2) * LOAD_FACTOR) {
			resize(s, s->bits - 1);
		}
	}
}

void
sparse_free(Sparse *s)
{
	free(s->slots);
}

