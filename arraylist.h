typedef struct {
    int *data;
    unsigned length;
    unsigned capacity;
} arraylist_t;

void al_init(arraylist_t *, unsigned);
void al_destroy(arraylist_t *);

unsigned al_length(arraylist_t *);

void al_push(arraylist_t *, int);
int al_pop(arraylist_t *, int *);


