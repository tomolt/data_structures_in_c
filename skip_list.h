#ifdef SKIP_LIST_H
#  error "Multiple inclusion."
#endif
#define SKIP_LIST_H

#define SL_NUM_LEVELS 6

struct sl_node {
	struct sl_node *next[SL_NUM_LEVELS];
	uintptr_t key;
	uintptr_t value;
};

struct skip_list {
	struct sl_node head;
	struct sl_node *free_list;
};

void sl_init(struct skip_list *sl);
void sl_inject_page(struct skip_list *sl, void *page);
void sl_walk(struct skip_list *sl, uintptr_t key, struct sl_node *frontier[]);
bool sl_find(struct skip_list *sl, uintptr_t key, uintptr_t *value);
void sl_insert_at(struct sl_node **free_list, struct sl_node *frontier[], uintptr_t key, uintptr_t value);
void sl_insert(struct skip_list *sl, uintptr_t key, uintptr_t value);
void sl_delete_at(struct sl_node **free_list, struct sl_node *frontier[]);
bool sl_delete(struct skip_list *sl, uintptr_t key);

