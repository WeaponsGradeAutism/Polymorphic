#include <Win32/wsock_datatypes.h>

//---------------------------------------------------------------------------------------------------------------------------------------------
//CONNECTION self-filling array

void connection_array_init(connection_array *vector) 
{
	// initialize size and capacity
	vector->connections.size = 0;
	vector->connections.members = 0;
	vector->connections.capacity = VECTOR_INITIAL_CAPACITY;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;

	// allocate memory for vector->data
	vector->connections.data = malloc(sizeof(CONNECTION) * vector->connections.capacity);
	vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);

	// need to set socket to 0 because it's used to indicate empty elements
	for (int x = 0; x < VECTOR_INITIAL_CAPACITY; x++)
	{
		vector->connections.data[x].socket = 0;
	}
}

void connection_array_init_capacity(connection_array *vector, uint32_t capacity) 
{
	// initialize size and capacity
	// initialize size and capacity
	vector->connections.size = 0;
	vector->connections.members = 0;
	vector->connections.capacity = capacity;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = capacity;

	// allocate memory for vector->data
	vector->connections.data = malloc(sizeof(CONNECTION) * vector->connections.capacity);
	vector->vacancies.data = malloc(sizeof(uint32_t) * vector->vacancies.capacity);

	// need to set socket to 0 because it's used to indicate empty elements
	for (unsigned int x = 0; x < capacity; x++)
	{
		vector->connections.data[x].socket = 0;
	}
}

void connection_array_double_capacity_if_full(connection_array *vector) 
{
	if (vector->connections.capacity == 0)
	{
		vector->connections.capacity = VECTOR_INITIAL_CAPACITY;
		vector->connections.data = malloc(sizeof(CONNECTION) * vector->connections.capacity);
	}
	if (vector->connections.size >= vector->connections.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		int oldCapacity = vector->connections.capacity;
		vector->connections.capacity *= 2;
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION) * vector->connections.capacity);

		// need to set socket to 0 because it's used to indicate empty elements
		for (unsigned int x = oldCapacity; x < vector->connections.capacity; x++)
		{
			vector->connections.data[x].socket = 0;
		}
	}
}

void connection_array_double_vacancy_capacity_if_full(connection_array *vector) 
{
	if (vector->vacancies.capacity == 0)
	{
		vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;
		vector->vacancies.data = malloc(sizeof(uint32_t) * vector->vacancies.capacity);
	}
	if (vector->vacancies.size >= vector->vacancies.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->vacancies.capacity *= 2;
		vector->vacancies.data = realloc(vector->vacancies.data, sizeof(uint32_t) * vector->vacancies.capacity);
	}
}

int connection_array_append(connection_array *vector, CONNECTION connection)
{

	// make sure there's room to expand into
	connection_array_double_capacity_if_full(vector);

	// append the value and increment vector->size
	int index = vector->connections.size++;
	vector->connections.members++;
	vector->connections.data[index] = connection;
	return index;
}

int connection_array_pop_vacancy(connection_array *vector)
{
	int ret = vector->vacancies.data[vector->vacancies.size - 1];
	vector->vacancies.size--;
	return ret;
}

int connection_array_push(connection_array *vector, CONNECTION connection, CONNECTION **out_connectionPointer)
{

	if (vector->vacancies.size > 0)
	{
		int index = connection_array_pop_vacancy(vector);
		vector->connections.data[index] = connection;
		*out_connectionPointer = &vector->connections.data[index];
		vector->connections.members++;
		return index;
	}
	else
	{
		return connection_array_append(vector, connection);
	}

}

int connection_array_element_exists(connection_array *vector, uint32_t index)
{
	if (index > 0 && index < vector->connections.size && vector->connections.data[index].socket != 0)
		return 1;
	else
		return 0;
}

int connection_array_delete(connection_array *vector, uint32_t index)
{
	if (index >= vector->connections.size || index < 0 || vector->connections.data[index].socket == 0)
		return VECTOR_INVALID_ARGUMENT;

	connection_array_double_vacancy_capacity_if_full(vector);
	int_vector_append(&vector->vacancies, index);
	vector->connections.data[index].socket = 0;
	vector->connections.members--;

	return 0;
}

CONNECTION * connection_array_get(connection_array *vector, uint32_t index)
{
	if (index >= vector->connections.size || index < 0)
		return NULL;

	return &vector->connections.data[index];
}

int connection_array_get_all(connection_array *vector, uint32_t maxCount, CONNECTION **OUT_connectionArray)
{
	uint32_t currentIndex = 0;
	for (unsigned int x = 0; x < vector->connections.size && currentIndex < maxCount; x++)
	{
		CONNECTION *connection = connection_array_get(vector, x);
		if (connection != NULL)
		{
			OUT_connectionArray[currentIndex] = connection;
			currentIndex++;
			if (currentIndex == maxCount)
			{
				if (x == (vector->connections.size - 1))
					return currentIndex;
				else
					return POLYM_SERVICE_ARRAY_GETALL_OVERFLOW;
			}
		}
	}
	return currentIndex;
}

void connection_array_trim(connection_array *vector) 
{
	//trim capacity down to the size of the array
	if (vector->connections.size == 0)
	{
		//if there are no elements, free the memory and create a null pointer
		free(vector->connections.data);
		vector->connections.data = NULL;
		vector->connections.capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION*) * vector->connections.size);
		vector->connections.capacity = vector->connections.size;

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
		vector->vacancies.data = realloc(vector->vacancies.data, sizeof(CONNECTION**) * vector->vacancies.size);
		vector->vacancies.capacity = vector->vacancies.size;
	}

}
int connection_array_free_internals(connection_array *vector)
{

	// all connections need to be freed and deleted before freeing the array
	if (vector->connections.members > 0)
		return ARRAY_NOT_EMPTY;

	free(vector->connections.data);
	free(vector->vacancies.data);
	return 0;
}

int connection_array_free(connection_array *vector) 
{

	// all connections need to be freed and deleted before freeing the array
	if (vector->connections.members > 0)
		return ARRAY_NOT_EMPTY;

	connection_array_free_internals(vector);
	free(vector);
	return 0;
}

//
//---------------------------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------------------------
//SERVICE CONNECTION self-filling array

void service_connection_array_init(service_connection_array *vector) 
{
	// initialize size and capacity
	vector->connections.size = 0;
	vector->connections.members = 0;
	vector->connections.capacity = VECTOR_INITIAL_CAPACITY;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;

	// allocate memory for vector->data
	vector->connections.data = malloc(sizeof(CONNECTION) * vector->connections.capacity);
	vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
	vector->connections.auxConnectionContainers = malloc(sizeof(connection_array) * vector->vacancies.capacity);

	// need to set socket to 0 because it's used to indicate empty elements
	for (int x = 0; x < VECTOR_INITIAL_CAPACITY; x++)
	{
		vector->connections.data[x].socket = 0;
	}
}

void service_connection_array_init_capacity(service_connection_array *vector, uint32_t capacity)
{
	// initialize size and capacity
	vector->connections.size = 0;
	vector->connections.members = 0;
	vector->connections.capacity = capacity;
	vector->vacancies.size = 0;
	vector->vacancies.capacity = capacity;

	// allocate memory for vector->data
	vector->connections.data = malloc(sizeof(CONNECTION) * vector->connections.capacity);
	vector->vacancies.data = malloc(sizeof(int) * vector->vacancies.capacity);
	vector->connections.auxConnectionContainers = malloc(sizeof(connection_array) * vector->vacancies.capacity);

	// need to set socket to 0 for each element because it's used to indicate empty elements
	for (unsigned int x = 0; x < capacity; x++)
	{
		vector->connections.data[x].socket = 0;
	}
}

void service_connection_array_double_capacity_if_full(service_connection_array *vector) 
{
	if (vector->connections.capacity == 0)
	{
		vector->connections.capacity = VECTOR_INITIAL_CAPACITY;
		vector->connections.data = malloc(sizeof(CONNECTION) * vector->connections.capacity);
		vector->connections.auxConnectionContainers = malloc(sizeof(connection_array) * vector->connections.capacity);

		// need to set socket to 0 for each new element because it's used to indicate empty elements
		for (int x = 0; x < VECTOR_INITIAL_CAPACITY; x++)
		{
			vector->connections.data[x].socket = 0;
		}
	}
	if (vector->connections.size >= vector->connections.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		int oldCapacity = vector->connections.capacity;
		vector->connections.capacity *= 2;
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION) * vector->connections.capacity);
		vector->connections.auxConnectionContainers = realloc(vector->connections.data, sizeof(connection_array) * vector->connections.capacity);

		// need to set socket to 0 for each new element because it's used to indicate empty elements
		for (unsigned int x = oldCapacity; x < vector->connections.capacity; x++)
		{
			vector->connections.data[x].socket = 0;
		}
	}
}

void service_connection_array_double_vacancy_capacity_if_full(service_connection_array *vector) 
{
	if (vector->vacancies.capacity == 0)
	{
		vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;
		vector->vacancies.data = malloc(sizeof(uint32_t) * vector->vacancies.capacity);
	}
	if (vector->vacancies.size >= vector->vacancies.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->vacancies.capacity *= 2;
		vector->vacancies.data = realloc(vector->vacancies.data, sizeof(uint32_t) * vector->vacancies.capacity);
	}
}

int service_connection_array_append(service_connection_array *vector, CONNECTION connection, CONNECTION **out_connectionPointer)
{

	// make sure there's room to expand into
	service_connection_array_double_capacity_if_full(vector);

	// append the value and increment vector->size
	int index = vector->connections.size++;
	vector->connections.members++;
	vector->connections.data[index] = connection;
	*out_connectionPointer = &vector->connections.data[index];
	connection_array_init(&vector->connections.auxConnectionContainers[index]);
	return index;
}

int service_connection_array_pop_vacancy(service_connection_array *vector)
{
	int ret = vector->vacancies.data[vector->vacancies.size - 1];
	vector->vacancies.size--;
	return ret;
}

int service_connection_array_push(service_connection_array *vector, CONNECTION connection, CONNECTION **out_connectionPointer)
{

	if (vector->vacancies.size > 0)
	{
		int index = service_connection_array_pop_vacancy(vector);
		vector->connections.data[index] = connection;
		*out_connectionPointer = &vector->connections.data[index];
		connection_array_init(&vector->connections.auxConnectionContainers[index]);
		vector->connections.members++;
		return index;
	}
	else
	{
		return service_connection_array_append(vector, connection, out_connectionPointer);
	}
}

int service_connection_array_push_aux(service_connection_array *vector, int index, CONNECTION connection, CONNECTION **out_connectionPointer)
{
	connection_array *auxconnList = &vector->connections.auxConnectionContainers[index];

	if (auxconnList->connections.members == 65535)
		return ARRAY_IS_FULL;

	return connection_array_push(auxconnList, connection, out_connectionPointer);
}

int service_connection_array_element_exists(service_connection_array *vector, uint32_t index)
{
	if (index > 0 && index < vector->connections.size && vector->connections.data[index].socket != 0)
		return 1;
	else
		return 0;
}

int service_connection_array_delete(service_connection_array *vector, uint32_t index)
{
	if (index >= vector->connections.size || index < 0 || vector->connections.data[index].socket == 0)
		return VECTOR_INVALID_ARGUMENT;

	// all aux connections need to be freed before the connection can be deleted
	if (ARRAY_NOT_EMPTY == connection_array_free_internals(&vector->connections.auxConnectionContainers[index]))
		return ARRAY_NOT_EMPTY;

	service_connection_array_double_vacancy_capacity_if_full(vector);
	int_vector_append(&vector->vacancies, index);
	vector->connections.data[index].socket = 0;
	vector->connections.members--;

	return 0;

	//ENHANCE: deleting the last element will cause the array size to shrink, and search the vancancy array for vancancies that can be release to shrink the array
}

int service_connection_array_delete_aux(service_connection_array *vector, uint32_t index, uint32_t indexAux)
{
	return connection_array_delete(&vector->connections.auxConnectionContainers[index], indexAux);

}

CONNECTION * service_connection_array_get_connection(service_connection_array *vector, uint32_t index) 
{
	if (index >= vector->connections.size || index < 0)
		return NULL;

	return &vector->connections.data[index];
}

int service_connection_array_get_all_connections(service_connection_array *vector, uint32_t maxCount, CONNECTION **OUT_connectionArray)
{
	uint32_t currentIndex = 0;
	for (unsigned int x = 0; x < vector->connections.size && currentIndex < maxCount; x++)
	{
		CONNECTION *connection = service_connection_array_get_connection(vector, x);
		if (connection != NULL)
		{
			OUT_connectionArray[currentIndex] = connection;
			currentIndex++;
			if (currentIndex == maxCount)
			{
				if (x == (vector->connections.size - 1))
					return currentIndex;
				else
					return POLYM_SERVICE_ARRAY_GETALL_OVERFLOW;
			}
		}
	}
	return currentIndex;
}

CONNECTION * service_connection_array_get_aux(service_connection_array *vector, uint32_t indexService, uint32_t indexAux) 
{
	if (indexService >= vector->connections.size || indexService < 0)
		return NULL;

	return connection_array_get(&vector->connections.auxConnectionContainers[indexService], indexAux);
}

connection_array * service_connection_array_get_auxlist(service_connection_array *vector, uint32_t index) {
	if (index >= vector->connections.size || index < 0)
		return NULL;

	return &vector->connections.auxConnectionContainers[index];
}


int service_connection_array_service_string_exists(service_connection_array *vector, char *string)
{
	for (unsigned int x = 0; x < vector->connections.size; x++)
	{
		CONNECTION *connection = service_connection_array_get_connection(vector, x);
		if (connection != NULL && 0 == strcmp(string, connection->info.mode_info.service.serviceString))
			return 1;
	}
	return 0;
}

void service_connection_array_trim(service_connection_array *vector) {
	//trim capacity down to the size of the array
	if (vector->connections.size == 0)
	{
		//if there are no elements, free the memory and create a null pointer
		free(vector->connections.data);
		vector->connections.data = NULL;
		free(vector->connections.auxConnectionContainers);
		vector->connections.auxConnectionContainers = NULL;
		vector->connections.capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION) * vector->connections.size);
		vector->connections.auxConnectionContainers = realloc(vector->connections.data, sizeof(connection_array) * vector->connections.size);
		vector->connections.capacity = vector->connections.size;
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

int service_connection_array_free(service_connection_array *vector) {

	// the connections need to be freed and deleted before the array can be freed
	if (vector->connections.members > 0)
		return ARRAY_NOT_EMPTY;

	free(vector->connections.data);
	free(vector->vacancies.data);
	free(vector->connections.auxConnectionContainers);
	free(vector);

	return 0;
}

//
//---------------------------------------------------------------------------------------------------------------------------------------------