#include <Win32/wsock_datatypes.h>

//---------------------------------------------------------------------------------------------------------------------------------------------
//POLYM_CONNECTION self-filling array

///<summary> Initialize the connection array. </summary>
void connection_array_init(connection_array *vector) 
{
	connection_array_init_capacity(vector, VECTOR_INITIAL_CAPACITY);
}

///<summary> Initialize the connection array at a given capacity. </summary>
void connection_array_init_capacity(connection_array *vector, int size) 
{
	// initialize size and capacity
	vector->size = size;
	vector->count = 0;
	vector->nextIndex = 0;

	// allocate memory for vector->data
	vector->connections = malloc(sizeof(POLYM_CONNECTION*) * size);

	// need to set socket to 0 because it's used to indicate empty elements
	for (int x = 0; x < size; x++)
	{
		vector->connections[x] = malloc(sizeof(POLYM_CONNECTION));
		vector->connections[x]->index = -1;
	}
}

///<summary> Expands the connection array by the given amount. </summary>
void connection_array_extend(connection_array *vector, unsigned int amount)
{
	int oldsize = vector->size;
	vector->size += amount;
	vector->connections = realloc(vector->connections, sizeof(POLYM_CONNECTION*) * vector->size);

	for (int x = oldsize - 1; x < vector->size; ++x)
	{
		vector->connections[x] = malloc(sizeof(POLYM_CONNECTION));
		vector->connections[x]->index = -1;
	}
}

///<summary> Pushes a copy of the connection object into the array. </summary>
int connection_array_push(connection_array *vector, POLYM_CONNECTION *connection, void **out_connectionPointer)
{
	for (int x = vector->nextIndex; x < vector->size; x++)
	{
		if (vector->connections[x]->index < 0) // empty slot is denoted by an index of < 0
		{
			vector->connections[x]->index = x;
			++vector->count;
			vector->nextIndex = x + 1;
			memcpy_s(vector->connections[x], sizeof(POLYM_CONNECTION), connection, sizeof(POLYM_CONNECTION));
			*out_connectionPointer = vector->connections[x];
			return x;
		}
	}

	// no open spaces found, expand the buffer for some more messages
	connection_array_extend(vector, 10);
	return connection_array_push(vector, connection, out_connectionPointer);
}

///<summary> Returns the connection object pointer for the given index. </summary>
POLYM_CONNECTION* connection_array_get(connection_array *vector, int index)
{
	if (index >= vector->size || index < 0 || vector->connections[index]->index < 0)
		return NULL;

	return vector->connections[index];
}

///<summary> Returns an array of array containing all connection objects, skipping empty elements. </summary>
///<returns> The size of the array produced. </returns>
int connection_array_get_all(connection_array *vector, int maxCount, POLYM_CONNECTION **out_connectionArray)
{
	int currentIndex = 0;
	for (int x = 0; x < vector->size && currentIndex < maxCount; ++x)
	{
		POLYM_CONNECTION *connection = connection_array_get(vector, x);
		if (connection != NULL)
		{
			
			out_connectionArray[currentIndex] = connection;
			currentIndex++;

			if (currentIndex == maxCount)
			{
				if (x == (vector->size - 1))
					return currentIndex;
				else
					return POLYM_SERVICE_ARRAY_GETALL_OVERFLOW;
			}
		}
	}
	return currentIndex;
}

///<summary> Marks the specified connection object as freed. </summary>
void connection_array_free(connection_array *vector, int index)
{
	vector->connections[index]->index = -1;
	--vector->count;
	if (index < vector->nextIndex)
	{
		vector->nextIndex = index;
	}
}

///<summary> Frees the message buffer array object. </summary>
void connection_array_free_container(connection_array *vector)
{
	free(vector->connections);
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

///<summary> Initializes the message buffer array at the default size. </summary>
void message_buffer_array_init(message_buffer_array *vector, int size)
{
	message_buffer_array_init_capacity(vector, VECTOR_INITIAL_CAPACITY);
}

///<summary> Initializes the message buffer array at the given size. </summary>
void message_buffer_array_init_capacity(message_buffer_array *vector, int size)
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

///<summary> Finds an unused message buffer in the array, initializes it, and returns a pointer to it. </summary>
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