//shamelessly stolen from https://www.happybearsoftware.com/implementing-a-dynamic-array

#pragma once

#include <stdbool.h>

#define VECTOR_INITIAL_CAPACITY 50

#define VECTOR_INVALID_ARGUMENT -1
#define ARRAY_NOT_EMPTY -2
#define ARRAY_IS_FULL -3

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

//string vector

typedef struct {
	int size;      // slots used so far
	int capacity;  // total available slots
	char **data;     // array of integers we're storing
} string_vector;

void string_vector_init(string_vector *vector);
void string_vector_init_capacity(string_vector *vector, int capacity);
void string_vector_append(string_vector *vector, char* value);
int string_vector_get(string_vector *vector, int index);
bool string_vector_set(string_vector *vector, int index, char* value);
void string_vector_trim(string_vector *vector);
void string_vector_free(string_vector *vector);

//Int vector

typedef struct {
	int size;      // the out bound of the array
	int count;     // the number of slots used
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

// Self-filling int array

typedef struct {
	int_vector members;
	int_vector vacancies;
} int_array;

void int_array_init(int_array *vector);
void int_array_init_capacity(int_array *vector, int capacity);
int int_array_append(int_array *vector, int value);
int int_array_push(int_array *vector, int value);
int int_array_delete(int_array *vector, int index);
int int_array_get(int_array *vector, int index);
int int_array_size(int_array *vector);
int int_array_count(int_array *vector);
bool int_array_set(int_array *vector, int index, int value);
void int_array_trim(int_array *vector);
void int_array_free(int_array *vector);