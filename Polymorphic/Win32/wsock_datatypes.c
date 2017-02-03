#include <Win32/wsock_datatypes.h>

//---------------------------------------------------------------------------------------------------------------------------------------------
//CONNECTION self-filling array

///<summary> Initialize the connection array. </summary>
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

///<summary> Initialize the connection array at a given capacity. </summary>
void connection_array_init_capacity(connection_array *vector, uint32_t capacity) 
{
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

///<summary> Grow the capacity of the connection array by doubling it. </summary>
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
		vector->connections.capacity += VECTOR_INITIAL_CAPACITY;
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION) * vector->connections.capacity);

		// need to set socket to 0 because it's used to indicate empty elements
		for (unsigned int x = oldCapacity; x < vector->connections.capacity; x++)
		{
			vector->connections.data[x].socket = 0;
		}
	}
}

// TODO: get rid of unneccessary vancancy tracking, replace with scan
void connection_array_double_vacancy_capacity_if_full(connection_array *vector) 
{
	if (vector->vacancies.capacity == 0)
	{
		vector->vacancies.capacity = VECTOR_INITIAL_CAPACITY;
		vector->vacancies.data = malloc(sizeof(uint32_t) * vector->vacancies.capacity);
	}
	if (vector->vacancies.size >= vector->vacancies.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		vector->vacancies.capacity += VECTOR_INITIAL_CAPACITY;
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

///<summary> Add a connection object to the array. </summary>
///<returns> Pointer to the connection object that's been added to the array. </returns>
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

///<summary> Check if a given array index contains a connection object. </summary>
int connection_array_element_exists(connection_array *vector, uint32_t index)
{
	if (index > 0 && index < vector->connections.size && vector->connections.data[index].socket != 0)
		return 1;
	else
		return 0;
}

///<summary> Deletes the connection object at the given index. </summary>
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

///<summary> Returns the connection object pointer for the given index. </summary>
CONNECTION * connection_array_get(connection_array *vector, uint32_t index)
{
	if (index >= vector->connections.size || index < 0)
		return NULL;

	return &vector->connections.data[index];
}

///<summary> Returns an array of array containing all connection objects, skipping empty elements. </summary>
//TODO: rename OUT
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

///<summary> Reduce the memory usage down to the smallest size possible. </summary>
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

int connection_array_free(connection_array *vector) 
{

	// all connections need to be freed and deleted before freeing the array
	if (vector->connections.members > 0)
		return ARRAY_NOT_EMPTY;

	free(vector->connections.data);
	free(vector->vacancies.data);
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

		// need to set socket to 0 for each new element because it's used to indicate empty elements
		for (int x = 0; x < VECTOR_INITIAL_CAPACITY; x++)
		{
			vector->connections.data[x].socket = 0;
		}
	}
	if (vector->connections.size >= vector->connections.capacity) {
		// double vector->capacity and resize the allocated memory accordingly
		int oldCapacity = vector->connections.capacity;
		vector->connections.capacity += VECTOR_INITIAL_CAPACITY;
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION) * vector->connections.capacity);

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
		vector->vacancies.capacity += VECTOR_INITIAL_CAPACITY;
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
		vector->connections.members++;
		return index;
	}
	else
	{
		return service_connection_array_append(vector, connection, out_connectionPointer);
	}
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

	service_connection_array_double_vacancy_capacity_if_full(vector);
	int_vector_append(&vector->vacancies, index);
	vector->connections.data[index].socket = 0;
	vector->connections.members--;

	return 0;

	//ENHANCE: deleting the last element will cause the array size to shrink, and search the vancancy array for vancancies that can be release to shrink the array
}

CONNECTION * service_connection_array_get_connection(service_connection_array *vector, uint32_t index) 
{
	if (index >= vector->connections.size || index < 0)
		return NULL;

	return &vector->connections.data[index];
}

int service_connection_array_get_all_connections(service_connection_array *vector, uint32_t maxCount, CONNECTION **OUT_connectionArray)
{ // TODO: Does the connection object store its connection ID?
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
		vector->connections.capacity = 0;
	}
	else
	{
		//size the memory allocation down to its size
		vector->connections.data = realloc(vector->connections.data, sizeof(CONNECTION) * vector->connections.size);
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
	free(vector);

	return 0;
}

//
//---------------------------------------------------------------------------------------------------------------------------------------------
// message buffer away
//---------------------------------------------------------------------------------------------------------------------------------------------

void message_buffer_array_init(message_buffer_array *vector, int size)
{
	vector->messages = malloc(sizeof(message_buffer) * size);
	vector->arraySize = size;
	vector->messageCount = 0;
	vector->nextIndex = 0;
}

// returns a pointer to a buffer that can be used to store data, and marks it as being in use by assigning it a length
message_buffer* message_buffer_array_allocate(message_buffer_array *vector, int length)
{
	for (int x = vector->nextIndex; x < vector->arraySize; x++)
	{
		if (vector->messages[x].index < 0) // empty slot is denoted by an index of < 0
		{
			vector->messages[x].index = x;
			vector->arraySize++;
			vector->nextIndex++;
			return &vector->messages[x];
		}
	}

	// no open spaces found, expand the buffer for some more messages
	//message_buffer_array_extend(vector, 10); TODO: implement
	int index = vector->nextIndex;
	vector->messages[index].index = vector->nextIndex;
	vector->arraySize++;
	vector->nextIndex++;
	return &vector->messages[index];
}

// marks a buffer as free once it's finished being used
void message_buffer_array_free(message_buffer_array *vector, int index)
{
	vector->messages[index].index = -1;
	vector->arraySize--;
}

void message_buffer_array_free_container(message_buffer_array *vector)
{
	free(vector->messages);
}