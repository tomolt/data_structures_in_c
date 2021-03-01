#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define LEFT(i)   (2 * (i) + 1)
#define RIGHT(i)  (2 * (i) + 2)
#define PARENT(i) (((i) - 1) / 2)
#define GRAND(i)  PARENT(PARENT(i))

#define MIN_IDX(h, a, b) ((h)->v[a] <= (h)->v[b] ? (a) : (b))
#define MAX_IDX(h, a, b) ((h)->v[a] >= (h)->v[b] ? (a) : (b))
#define SWAP(h, a, b) do {				\
		uintptr_t tmp = h->v[a];		\
		h->v[a] = h->v[b];				\
		h->v[b] = tmp;					\
	} while (0)

struct deheap {
	uintptr_t *v;
	size_t length;
	size_t capacity;
};

const size_t deheap_size = sizeof(struct deheap);

static bool
push_last(struct deheap *h, uintptr_t elem)
{
	void *mem;
	if (h->length >= h->capacity) {
		h->capacity = h->capacity ? 2 * h->capacity : 16;
		mem = realloc(h->v, h->capacity * sizeof(uintptr_t));
		if (!mem) {
			return false;
		}
		h->v = mem;
	}
	h->v[h->length++] = elem;
	return true;
}

static void
bubble_up_min(struct deheap *h, size_t idx)
{
	size_t grand;

	while (idx > 2) {
		grand = GRAND(idx);
		if (h->v[idx] >= h->v[grand]) break;
		SWAP(h, idx, grand);
		idx = grand;
	}
}

static void
bubble_up_max(struct deheap *h, size_t idx)
{
	size_t grand;

	while (idx > 2) {
		grand = GRAND(idx);
		if (h->v[idx] <= h->v[grand]) break;
		SWAP(h, idx, grand);
		idx = grand;
	}
}

static void
bubble_up(struct deheap *h, size_t idx)
{
	size_t parent;
	int layer;

	if (!idx) return;
	parent = PARENT(idx);

	layer = 63 - __builtin_clzl(idx + 1);
	if (layer & 1) {
		/* max layer */
		if (h->v[idx] < h->v[parent]) {
			SWAP(h, idx, parent);
			bubble_up_min(h, parent);
		} else {
			bubble_up_max(h, idx);
		}
	} else {
		/* min layer */
		if (h->v[idx] > h->v[parent]) {
			SWAP(h, idx, parent);
			bubble_up_max(h, parent);
		} else {
			bubble_up_min(h, idx);
		}
	}
}

static inline size_t
max_get_min(struct deheap *h, size_t idx)
{
	size_t left, right;

	left = LEFT(idx);
	right = RIGHT(idx);
	if (right < h->length) {
		/* we have two min children. */
		return MIN_IDX(h, left, right);
	} else if (left < h->length) {
		/* we have exactly one min child. */
		return left;
	} else {
		/* we don't have any children. */
		return idx;
	}
}

static void
trickle_down_min(struct deheap *h, size_t idx)
{
	size_t left, right, lmin, rmin, min;

	for (;;) {
		left = LEFT(idx);
		right = RIGHT(idx);
		if (right < h->length) {
			/* we have two max children. */
			lmin = max_get_min(h, left);
			rmin = max_get_min(h, right);
			min = MIN_IDX(h, lmin, rmin);
		} else if (left < h->length) {
			/* we have exactly one max child. */
			min = max_get_min(h, left);
		} else {
			/* we don't have any children. */
			break;
		}
		if (h->v[idx] <= h->v[min]) {
			/* heap invariant is already satisfied. */
			break;
		}
		SWAP(h, idx, min);
		idx = min;
	}
}

static inline size_t
min_get_max(struct deheap *h, size_t idx)
{
	size_t left, right;

	left = LEFT(idx);
	right = RIGHT(idx);
	if (right < h->length) {
		/* we have two max children. */
		return MAX_IDX(h, left, right);
	} else if (left < h->length) {
		/* we have exactly one max child. */
		return left;
	} else {
		/* we don't have any children. */
		return idx;
	}
}

static void
trickle_down_max(struct deheap *h, size_t idx)
{
	size_t left, right, lmax, rmax, max;

	for (;;) {
		left = LEFT(idx);
		right = RIGHT(idx);
		if (right < h->length) {
			/* we have two max children. */
			lmax = min_get_max(h, left);
			rmax = min_get_max(h, right);
			max = MAX_IDX(h, lmax, rmax);
		} else if (left < h->length) {
			/* we have exactly one max child. */
			max = min_get_max(h, left);
		} else {
			/* we don't have any children. */
			break;
		}
		if (h->v[idx] >= h->v[max]) {
			/* heap invariant is already satisfied. */
			break;
		}
		SWAP(h, idx, max);
		idx = max;
	}
}

void
deheap_init(struct deheap *h)
{
	memset(h, 0, sizeof(*h));
}

bool
deheap_duplicate(struct deheap *src, struct deheap *dst)
{
	dst->length = src->length;
	dst->capacity = src->capacity;
	dst->v = malloc(dst->capacity * sizeof(uintptr_t));
	if (!dst->v) {
		return false;
	}
	memcpy(dst->v, src->v, dst->length * sizeof(uintptr_t));
	return true;
}

bool
deheap_push(struct deheap *h, uintptr_t elem)
{
	if (!push_last(h, elem)) {
		return false;
	}
	bubble_up(h, h->length - 1);
	return true;
}

bool
deheap_pop_min(struct deheap *h, uintptr_t *elem)
{
	if (!h->length) {
		return false;
	}
	*elem = h->v[0];
	h->v[0] = h->v[--h->length];
	trickle_down_min(h, 0);
	return true;
}

bool
deheap_pop_max(struct deheap *h, uintptr_t *elem)
{
	size_t idx;

	if (h->length >= 3) {
		/* we have two max roots. */
		if (h->v[1] >= h->v[2]) {
			idx = 1;
		} else {
			idx = 2;
		}
	} else if (h->length == 2) {
		/* we have a single max root. */
		idx = 1;
	} else if (h->length) {
		/* we don't have any max roots. */
		idx = 0;
	} else {
		/* the heap is completely empty. */
		return false;
	}
	*elem = h->v[idx];
	h->v[idx] = h->v[--h->length];
	trickle_down_max(h, idx);
	return true;
}

#if 0
#include <stdio.h>

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

int
main()
{
	const uintptr_t values[] = { 46, 31, 51, 71, 31, 10, 21, 8, 13, 11, 41, 16 };
	struct deheap h, g;
	size_t i;

	printf("INPUT :");
	deheap_init(&h);
	for (i = 0; i < ARRAY_LENGTH(values); ++i) {
		printf(" %2lu", values[i]);
		deheap_push(&h, values[i]);
	}
	printf("\n");

	printf("HEAP  :");
	for (i = 0; i < h.length; ++i) {
		printf(" %2lu", h.v[i]);
	}
	printf("\n");

	deheap_duplicate(&h, &g);

	printf("POPMIN:");
	while (h.length) {
		uintptr_t min;
		deheap_pop_min(&h, &min);
		printf(" %2lu", min);
	}
	printf("\n");
	free(h.v);

	printf("POPMAX:");
	while (g.length) {
		uintptr_t max;
		deheap_pop_max(&g, &max);
		printf(" %2lu", max);
	}
	printf("\n");
	free(g.v);

	return 0;
}
#endif

