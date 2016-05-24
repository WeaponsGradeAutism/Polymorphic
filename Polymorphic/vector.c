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
	vector->data = (void**)malloc(sizeof(void*) * vector->capacity); //OPTI: able to remove cast?
}

void vector_init_capacity(vector *vector, int capacity) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = capacity;

	// allocate memory for vector->data
	vector->data = (void**)malloc(sizeof(void*) * vector->capacity); //OPTI: able to remove cast?
}

void vector_double_capacity_if_full(vector *vector) {
	if (vector->size >= vector->capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->capacity *= 2;
		vector->data = (void**)realloc(vector->data, sizeof(void*) * vector->capacity); //OPTI: able to remove cast?
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
	if (index >= vector->size) {
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
		vector->data = (void**)realloc(vector->data, sizeof(void*) * vector->size); //OPTI: able to remove cast?
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
	vector->data = (int*)malloc(sizeof(int) * vector->capacity); //OPTI: able to remove cast?
}

void int_vector_init_capacity(int_vector *vector, int capacity) {
	// initialize size and capacity
	vector->size = 0;
	vector->capacity = capacity;

	// allocate memory for vector->data
	vector->data = (int*)malloc(sizeof(int) * vector->capacity); //OPTI: able to remove cast?
}

void int_vector_double_capacity_if_full(int_vector *vector) {
	if (vector->size >= vector->capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->capacity *= 2;
		vector->data = (int*)realloc(vector->data, sizeof(int) * vector->capacity); //OPTI: able to remove cast?
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
	if (index >= vector->size) {
		return false;
	}

	// set the value at the desired index
	vector->data[index] = value;
	return true;
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
		vector->data = (int*)realloc(vector->data, sizeof(int) * vector->size); //OPTI: able to remove cast?
		vector->capacity = vector->size;
	}
}

void int_vector_free(int_vector *vector) {
	free(vector->data);
}