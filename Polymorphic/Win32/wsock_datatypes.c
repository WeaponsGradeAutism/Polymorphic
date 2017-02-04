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
///<returns> The size of the array produced. </returns>
//TODO: rename OUT
int connection_array_get_all(connection_array *vector, unsigned int maxCount, CONNECTION **OUT_connectionArray)
{
	unsigned int currentIndex = 0;
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

///<summary> Frees the provided connection array. Array must be empty.</summary>
//TODO: Add connection_array_empty method.
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
// message buffer array
//---------------------------------------------------------------------------------------------------------------------------------------------

///<summary> Initializes the message buffer, prepping it for use or re-use. </summary>
void message_buffer_init(message_buffer *buffer)
{
	buffer->overlap.eventInfo = NULL;
	buffer->overlap.eventType = 0;
	buffer->overlap.hEvent = 0;
	buffer->overlap.Internal = 0;
	buffer->overlap.InternalHigh = 0;
	buffer->overlap.Offset = 0;
	buffer->overlap.OffsetHigh = 0;
	buffer->overlap.Pointer = NULL;

	buffer->wsabuf.len = 0;
	buffer->wsabuf.buf = NULL;
}

///<summary> Initializes the message buffer array at the given size. </summary>
void message_buffer_array_init(message_buffer_array *vector, int size)
{
	vector->messages = malloc(sizeof(message_buffer*) * size);
	vector->size = size;
	vector->count = 0;
	vector->nextIndex = 0;

	for (int x = 0; x < size; ++x)
	{
		vector->messages[x] = malloc(sizeof(message_buffer));
		vector->messages[x]->index = -1;
	}
}

///<summary> Expands the message buffer array by the given amount. </summary>
void message_buffer_array_extend(message_buffer_array *vector, unsigned int amount)
{
	int oldsize = vector->size;
	vector->size += amount;
	vector->messages = realloc(vector, sizeof(message_buffer*) * vector->size);

	for (int x = oldsize - 1; x < vector->size; ++x)
	{
		vector->messages[x] = malloc(sizeof(message_buffer));
		vector->messages[x]->index = -1;
	}
}

///<summary> Finds an empty message buffer in the array, initialized it, and returns a pointer to it. </summary>
message_buffer* message_buffer_array_allocate(message_buffer_array *vector)
{
	for (int x = vector->nextIndex; x < vector->size; x++)
	{
		if (vector->messages[x]->index < 0) // empty slot is denoted by an index of < 0
		{
			vector->messages[x]->index = x;
			++vector->count;
			vector->nextIndex = x+1;
			message_buffer_init(vector->messages[x]);
			return vector->messages[x];
		}
	}

	// no open spaces found, expand the buffer for some more messages
	message_buffer_array_extend(vector, 10);
	return message_buffer_array_allocate(vector);
}

///<summary> Marks the specified message buffer as freed. </summary>
void message_buffer_array_free(message_buffer_array *vector, int index)
{
	vector->messages[index]->index = -1;
	--vector->count;
	if (index < vector->nextIndex)
	{
		vector->nextIndex = index;
	}
}

///<summary> Frees the message buffer array object. </summary>
void message_buffer_array_free_container(message_buffer_array *vector)
{
	free(vector->messages);
}