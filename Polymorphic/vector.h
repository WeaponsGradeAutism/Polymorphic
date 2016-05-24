//shamelessly stolen from https://www.happybearsoftware.com/implementing-a-dynamic-array

#include <stdbool.h>

#define VECTOR_INITIAL_CAPACITY 50

//Generic vector

typedef struct {
	int size;      // slots used so far
	int capacity;  // total available slots
	void **data;     // array of objects we're storing
} vector;

void vector_init(vector *vector);
void vector_init_capacity(vector *vector, int capacity);
void vector_append(vector *vector, void *value);
void* vector_get(vector *vector, int index);
bool vector_set(vector *vector, int index, void *value);
void vector_trim(vector *vector);
void vector_free(vector *vector);

//Int vector

typedef struct  {
	int size;      // slots used so far
	int capacity;  // total available slots
	int *data;     // array of integers we're storing
} int_vector;

void int_vector_init(int_vector *vector);
void int_vector_init_capacity(int_vector *vector, int capacity);
void int_vector_append(int_vector *vector, int value);
int int_vector_get(int_vector *vector, int index);
bool int_vector_set(int_vector *vector, int index, int value);
void int_vector_trim(int_vector *vector);
void int_vector_free(int_vector *vector);
