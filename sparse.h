#ifdef SPARSE_H
#  error "Multiple inclusion."
#endif
#define SPARSE_H

typedef struct Sparse Sparse;

struct SparseSlot {
	uintmax_t key;
	intptr_t  psl; /* probe sequence length, aka distance from optimal slot */
	void     *val;
};

struct Sparse {
	struct SparseSlot *slots;
	size_t load; /* how many slots are used */
	size_t max;
	int    bits; /* logarithm of the capacity of the table */
};

void  sparse_init(Sparse *s, int bits);
void *sparse_get (Sparse *s, uintmax_t key);
void  sparse_set (Sparse *s, uintmax_t key, void *value);
void  sparse_free(Sparse *s);

