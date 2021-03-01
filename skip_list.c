#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "skip_list.h"

#define PAGE_SIZE 4096

int arch_bsf32(uint32_t bits);
void panic(const char *msg);

/* TODO Get a proper CRNG going. */
static uint32_t
awful_rng(void)
{
	const uint64_t FACTOR_L = 0x995deb95;
	const uint64_t FACTOR_M = 0x60b11728;
	const uint64_t FACTOR_H = 0xdc879768;
	static uint32_t seed_l = 1;
	static uint32_t seed_m = 0;
	static uint32_t seed_h = 0;
	uint64_t prod_l, prod_m, prod_h;

	prod_l = FACTOR_L * seed_l;
	prod_m = FACTOR_L * seed_m + FACTOR_M * seed_l + (prod_l >> 32);
	prod_h = FACTOR_L * seed_h + FACTOR_M * seed_m + FACTOR_H * seed_l + (prod_m >> 32);

	seed_l = prod_l;
	seed_m = prod_m;
	seed_h = prod_h;

	return seed_h;
}

static int
choose_height(void)
{
	uint32_t bits = awful_rng();
	bits |= 1 << (SL_NUM_LEVELS - 1);
	return arch_bsf32(bits);
}

static void
inject_free_list(struct sl_node **free_list, void *page)
{
	struct sl_node *nodes = page;
	struct sl_node *first_free = *free_list;
	unsigned long i;

	for (i = 0; i < PAGE_SIZE / sizeof(struct sl_node); ++i) {
		nodes[i].next[0] = first_free;
		first_free = &nodes[i];
	}
	*free_list = first_free;
}

static void
grow_free_list(struct sl_node **free_list)
{
	(void) free_list;
	panic("skip list memory is too small.");
}

static struct sl_node *
alloc_node(struct sl_node **free_list)
{
	struct sl_node *node;
	if (!*free_list) {
		grow_free_list(free_list);
	}
	node = *free_list;
	*free_list = node->next[0];
	return node;
}

static void
free_node(struct sl_node **free_list, struct sl_node *node)
{
	node->next[0] = *free_list;
	*free_list = node;
}

void
sl_init(struct skip_list *sl)
{
	memset(sl, 0, sizeof(*sl));
}

void
sl_inject_page(struct skip_list *sl, void *page)
{
	inject_free_list(&sl->free_list, page);
}

void
sl_walk(struct skip_list *sl, uintptr_t key, struct sl_node *frontier[])
{
	struct sl_node *node = &sl->head, *next;
	int level;

	for (level = SL_NUM_LEVELS - 1; level >= 0; --level) {
		next = node->next[level];
		while (next && next->key < key) {
			node = next;
			next = node->next[level];
		}
		frontier[level] = node;
	}
}

bool
sl_find(struct skip_list *sl, uintptr_t key, uintptr_t *value)
{
	struct sl_node *frontier[SL_NUM_LEVELS], *node;

	sl_walk(sl, key, frontier);
	node = frontier[0]->next[0];
	if (node && node->key == key) {
		*value = node->value;
		return true;
	} else {
		return false;
	}
}

void
sl_insert_at(struct sl_node **free_list, struct sl_node *frontier[], uintptr_t key, uintptr_t value)
{
	struct sl_node *node, *a, *b;
	int height = choose_height(), level;

	node = alloc_node(free_list);
	node->key = key;
	node->value = value;

	for (level = 0; level <= height; ++level) {
		a = frontier[level];
		b = a->next[level];
		a->next[level] = node;
		node->next[level] = b;
	}
}

void
sl_insert(struct skip_list *sl, uintptr_t key, uintptr_t value)
{
	struct sl_node *frontier[SL_NUM_LEVELS];

	sl_walk(sl, key, frontier);
	sl_insert_at(&sl->free_list, frontier, key, value);
}

void
sl_delete_at(struct sl_node **free_list, struct sl_node *frontier[])
{
	struct sl_node *node, *a, *b, *c;
	int level;

	node = frontier[0]->next[0];
	for (level = 0; level < SL_NUM_LEVELS; ++level) {
		a = frontier[level];
		b = a->next[level];
		if (b != node) break;
		c = b->next[level];
		a->next[level] = c;
	}
	free_node(free_list, node);
}

bool
sl_delete(struct skip_list *sl, uintptr_t key)
{
	struct sl_node *frontier[SL_NUM_LEVELS], *node;

	sl_walk(sl, key, frontier);
	node = frontier[0]->next[0];
	if (node && node->key == key) {
		sl_delete_at(&sl->free_list, frontier);
		return true;
	} else {
		return false;
	}
}

