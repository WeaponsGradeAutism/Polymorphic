//shamelessly stolen from https://www.happybearsoftware.com/implementing-a-dynamic-array

#include <stdio.h>
#include <stdlib.h>
#include <vector.h>

//generic_vector

void vector_init(vector *vector) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = VECTOR_INITIAL_CAPACITY;

	// allocate memory for vector->data
	vector->data = malloc(sizeof(void*) * vector->capacity);
}

void vector_init_capacity(vector *vector, int capacity) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = capacity;

	// allocate memory for vector->data
	vector->data = malloc(sizeof(void*) * vector->capacity);
}

void vector_double_capacity_if_full(vector *vector) {
	if (vector->capacity == 0)
	{
		vector->capacity = VECTOR_INITIAL_CAPACITY;
		vector->data = malloc(sizeof(void*) * vector->capacity);
	}
	if (vector->size >= vector->capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->capacity *= 2;
		vector->data = realloc(vector->data, sizeof(void*) * vector->capacity);
	}
}

void vector_append(vector *vector, void *value) {
	// make sure there's room to expand into
	vector_double_capacity_if_full(vector);

	// append the value and increment vector->size
	vector->data[vector->size++] = value;
}

void* vector_get(vector *vector, int index) {
	if (index >= vector->size || index < 0) {
		printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
		exit(1);
	}
	return vector->data[index];
}

bool vector_set(vector *vector, int index, void *value) {
	// fail if outside bounds
	if (index >= vector->size || index < 0) {
		return false;
	}

	// set the value at the desired index
	vector->data[index] = value;
	return true;
}

void vector_trim(vector *vector) {
	if (vector->size == 0)
	{
		free(vector->data);
		vector->data = NULL;
		vector->capacity = 0;
	}
	else
	{
		vector->data = realloc(vector->data, sizeof(void*) * vector->size);
		vector->capacity = vector->size;
	}
}

void vector_free(vector *vector) {
	free(vector->data);
}


//---------------------------------------------------------------------------------------------------------------------------------------------
//Int vector


void int_vector_init(int_vector *vector) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = VECTOR_INITIAL_CAPACITY;

	// allocate memory for vector->data
	vector->data = malloc(sizeof(int) * vector->capacity);
}

void int_vector_init_capacity(int_vector *vector, int capacity) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = capacity;

	// allocate memory for vector->data
	vector->data = malloc(sizeof(int) * vector->capacity);
}

void int_vector_double_capacity_if_full(int_vector *vector) {
	if (vector->capacity == 0)
	{
		vector->capacity = VECTOR_INITIAL_CAPACITY;
		vector->data = malloc(sizeof(int) * vector->capacity);
	}
	if (vector->size >= vector->capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->capacity *= 2;
		vector->data = realloc(vector->data, sizeof(int) * vector->capacity);
	}
}

void int_vector_append(int_vector *vector, int value) {
	// make sure there's room to expand into
	int_vector_double_capacity_if_full(vector);

	// append the value and increment vector->size
	vector->data[vector->size++] = value;
}

int int_vector_get(int_vector *vector, int index) {
	if (index >= vector->size || index < 0) {
		printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
		exit(1);
	}
	return vector->data[index];
}

bool int_vector_set(int_vector *vector, int index, int value) {
	// fail if out of bounds
	if (index >= vector->size || index < 0) {
		return false;
	}

	// set the value at the desired index
	vector->data[index] = value;
	return true;
}

int int_vector_find(int_vector *vector, int value)
{
	for (int index = 0; index < vector->size; index++)
	{
		if (int_vector_get(vector, index) == value)
		{
			return index;
		}
	}
	return -1;
}

void int_vector_trim(int_vector *vector) {
	//trim capacity down to the size of the array
	if (vector->size == 0) 
	{
		//if there are no elements, free the memory and create a null pointer
		free(vector->data);
		vector->data = NULL;
		vector->capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->data = realloc(vector->data, sizeof(int) * vector->size);
		vector->capacity = vector->size;
	}
}

void int_vector_free(int_vector *vector) {
	free(vector->data);
}


//---------------------------------------------------------------------------------------------------------------------------------------------
//Int self-filling array


void int_array_init(int_array *vector) {
	// initialize size and capacity
	vector->members.size = 0;
	vector->members.capacity = VECTOR_INITIAL_CAPACITY;
	vector->members.count = 0;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;

	// allocate memory for vector->data
	vector->members.data = malloc(sizeof(int) * vector->members.capacity);
	vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
}

void int_array_init_capacity(int_array *vector, int capacity) {
	// initialize size and capacity
	vector->members.size = 0;
	vector->members.capacity = capacity;
	vector->members.count = 0;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = capacity;

	// allocate memory for vector->data
	vector->members.data = malloc(sizeof(int) * vector->members.capacity);
	vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
}

void int_array_double_capacity_if_full(int_array *vector) {
	if (vector->members.capacity == 0)
	{
		vector->members.capacity = VECTOR_INITIAL_CAPACITY;
		vector->members.data = malloc(sizeof(int) * vector->members.capacity);
	}
	if (vector->members.size >= vector->members.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->members.capacity *= 2;
		vector->members.data = realloc(vector->members.data, sizeof(int) * vector->members.capacity);
	}
}

void int_array_double_vacancy_capacity_if_full(int_array *vector) {
	if (vector->vacancies.capacity == 0)
	{
		vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;
		vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
	}
	if (vector->vacancies.size >= vector->vacancies.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->vacancies.capacity *= 2;
		vector->vacancies.data = realloc(vector->vacancies.data, sizeof(int) * vector->vacancies.capacity);
	}
}

int int_array_append(int_array *vector, int value) 
{
	if (value == INT_MIN)
		return -1;

	// make sure there's room to expand into
	int_array_double_capacity_if_full(vector);

	// append the value and increment vector->size
	int index = vector->members.size++;
	vector->members.data[index] = value;
	vector->members.count++;
	return index;
}

int int_array_get_vacancy(int_array *vector)
{
	return vector->vacancies.data[vector->vacancies.size - 1];
}

void int_array_pop_vacancy(int_array *vector)
{
	vector->vacancies.data[vector->vacancies.size - 1] = INT_MIN;
	vector->vacancies.size--;
}

int int_array_push(int_array *vector, int value) {

	if (value == INT_MIN)
		return -1;

	if (vector->vacancies.size > 0)
	{
		int index = int_array_get_vacancy(vector);
		vector->members.data[index] = value;
		vector->members.count++;
		int_array_pop_vacancy(vector);
		return index;
	}
	else
	{
		return int_array_append(vector, value);
	}

}

int int_array_delete(int_array *vector, int index)
{
	if (index >= vector->members.size || index < 0 || vector->members.data[index] == INT_MIN)
		return -1;

	int_array_double_vacancy_capacity_if_full(vector);
	vector->vacancies.data[vector->vacancies.size++] = index;
	vector->members.data[index] = INT_MIN;
	vector->members.count--;

	return 0;
}

int int_array_get(int_array *vector, int index) {
	if (index >= vector->members.size || index < 0) {
		printf("Index %d out of bounds for vector of size %d\n", index, vector->members.size);
		exit(1);
	}

	return vector->members.data[index];
}


int int_array_find(int_array *vector, int value)
{
	for (int index = 0; index < vector->members.size; index++)
	{
		if (int_array_get(vector, index) == value)
		{
			return index;
		}
	}
	return -1;
}

int int_array_size(int_array *vector) 
{
	return vector->members.size;
}
int int_array_count(int_array *vector) 
{
	return vector->members.count;
}

bool int_array_set(int_array *vector, int index, int value) {
	// fail if out of bounds

	if (value == INT_MIN)
		return -1;

	if (vector->members.data[index] == INT_MIN)
		return -1;

	if (index >= vector->members.size || index < 0) {
		return false;
	}

	// set the value at the desired index
	vector->members.data[index] = value;
	return true;
}

void int_array_trim(int_array *vector) {
	//trim capacity down to the size of the array
	if (vector->members.size == 0)
	{
		//if there are no elements, free the memory and create a null pointer
		free(vector->members.data);
		vector->members.data = NULL;
		vector->members.capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->members.data = realloc(vector->members.data, sizeof(int) * vector->members.size);
		vector->members.capacity = vector->members.size;
	}

	if (vector->vacancies.size == 0)
	{
		//if there are no elements, free the memory and create a null pointer
		free(vector->vacancies.data);
		vector->vacancies.data = NULL;
		vector->vacancies.capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->vacancies.data = realloc(vector->vacancies.data, sizeof(int) * vector->vacancies.size);
		vector->vacancies.capacity = vector->vacancies.size;
	}

}

void int_array_free(int_array *vector) {
	free(vector->members.data);
	free(vector->vacancies.data);
}