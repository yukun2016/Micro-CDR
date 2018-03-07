// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "microcdr/microcdr.h"

#include <string.h>
#include <stdlib.h>

#if __BIG_ENDIAN__
    const Endianness MACHINE_ENDIANNESS = BIG_ENDIANNESS;
#else
    const Endianness MACHINE_ENDIANNESS = LITTLE_ENDIANNESS;
#endif

// -------------------------------------------------------------------
//                  INTERNAL SERIALIZATION FUNCTIONS
// -------------------------------------------------------------------

// NOTE: remove inline from byte's functions will reduce significally the size of the compiler data.
//          put inline from byte's functions will increase the performance.

inline static bool serialize_byte_1(MicroBuffer* buffer, const uint8_t* byte);
inline static bool serialize_byte_2(MicroBuffer* buffer, Endianness endianness, const uint16_t* bytes);
inline static bool serialize_byte_4(MicroBuffer* buffer, Endianness endianness, const uint32_t* bytes);
inline static bool serialize_byte_8(MicroBuffer* buffer, Endianness endianness, const uint64_t* bytes);

inline static bool deserialize_byte_1(MicroBuffer* buffer, uint8_t* byte);
inline static bool deserialize_byte_2(MicroBuffer* buffer, Endianness endianness, uint16_t* bytes);
inline static bool deserialize_byte_4(MicroBuffer* buffer, Endianness endianness, uint32_t* bytes);
inline static bool deserialize_byte_8(MicroBuffer* buffer, Endianness endianness, uint64_t* bytes);

inline static bool serialize_array_byte_1(MicroBuffer* buffer, const uint8_t* array, uint32_t size);
inline static bool serialize_array_byte_2(MicroBuffer* buffer, Endianness endianness, const uint16_t* array, uint32_t size);
inline static bool serialize_array_byte_4(MicroBuffer* buffer, Endianness endianness, const uint32_t* array, uint32_t size);
inline static bool serialize_array_byte_8(MicroBuffer* buffer, Endianness endianness, const uint64_t* array, uint32_t size);

inline static bool deserialize_array_byte_1(MicroBuffer* buffer, uint8_t* array, uint32_t size);
inline static bool deserialize_array_byte_2(MicroBuffer* buffer, Endianness endianness, uint16_t* array, uint32_t size);
inline static bool deserialize_array_byte_4(MicroBuffer* buffer, Endianness endianness, uint32_t* array, uint32_t size);
inline static bool deserialize_array_byte_8(MicroBuffer* buffer, Endianness endianness, uint64_t* array, uint32_t size);

inline static bool deserialize_inline_array_byte_1(MicroBuffer* buffer, uint8_t** array, uint32_t size);
inline static bool deserialize_inline_array_byte_2(MicroBuffer* buffer, Endianness endianness, uint16_t** array, uint32_t size);
inline static bool deserialize_inline_array_byte_4(MicroBuffer* buffer, Endianness endianness, uint32_t** array, uint32_t size);
inline static bool deserialize_inline_array_byte_8(MicroBuffer* buffer, Endianness endianness, uint64_t** array, uint32_t size);

// -------------------------------------------------------------------
//                      INTERNAL UTIL FUNCTIONS
// -------------------------------------------------------------------
inline static uint32_t get_alignment_offset(MicroBuffer* buffer, uint32_t data_size);
inline static bool check_size(MicroBuffer* buffer, uint32_t bytes);

// -------------------------------------------------------------------
//                 BUFFER MANAGEMENT IMPLEMENTATION
// -------------------------------------------------------------------

void init_micro_buffer(MicroBuffer* buffer, uint8_t* data, uint32_t size)
{
    buffer->init = data;
    buffer->final = buffer->init + size;
    buffer->iterator = buffer->init;
    buffer->last_data_size = 0;
    buffer->endianness = BIG_ENDIANNESS;
    buffer->error = BUFFER_OK;
}

void reset_micro_buffer(MicroBuffer* buffer)
{
    buffer->iterator = buffer->init;
    buffer->last_data_size = 0;
    buffer->error = BUFFER_OK;
}

MicroState get_micro_state(MicroBuffer* buffer)
{
    return (MicroState)
    {
        .position = buffer->iterator,
        .last_data_size = buffer->last_data_size,
        .error = buffer->error
    };
}

void restore_micro_state(MicroBuffer* buffer, MicroState state)
{
    buffer->iterator = state.position;
    buffer->last_data_size = state.last_data_size;
    buffer->error = state.error;
}

void align_to(MicroBuffer* buffer, uint32_t size)
{
    buffer->iterator += get_alignment_offset(buffer, size);
    buffer->last_data_size = size;
}

uint32_t get_alignment_offset(MicroBuffer* buffer, uint32_t data_size)
{
    if(data_size > buffer->last_data_size)
        return (data_size - ((buffer->iterator - buffer->init) % data_size)) & (data_size - 1);

    return 0;
}

bool check_size(MicroBuffer* buffer, uint32_t bytes)
{
    return buffer->iterator + bytes <= buffer->final;
}

// -------------------------------------------------------------------
//                  SERIALIZATION IMPLEMENTATION
// -------------------------------------------------------------------
bool serialize_byte_1(MicroBuffer* buffer, const uint8_t* byte)
{
    uint32_t data_size = sizeof(uint8_t);
    if(check_size(buffer, data_size))
    {
        *buffer->iterator = *byte;

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_byte_2(MicroBuffer* buffer, Endianness endianness, const uint16_t* bytes)
{
    uint32_t data_size = sizeof(uint16_t);
    uint32_t alignment = get_alignment_offset(buffer, data_size);

    if(check_size(buffer, alignment + data_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
            *((uint16_t*)buffer->iterator) = *bytes;
        else
        {
            uint8_t* bytes_pointer = (uint8_t*)bytes;
            *buffer->iterator = *(bytes_pointer + 1);
            *(buffer->iterator + 1) = *bytes_pointer;
        }

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_byte_4(MicroBuffer* buffer, Endianness endianness, const uint32_t* bytes)
{
    uint32_t data_size = sizeof(uint32_t);
    uint32_t alignment = get_alignment_offset(buffer, data_size);

    if(check_size(buffer, alignment + data_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
            *((uint32_t*)buffer->iterator) = *bytes;
        else
        {
            uint8_t* bytes_pointer = (uint8_t*)bytes;
            *buffer->iterator = *(bytes_pointer + 3);
            *(buffer->iterator + 1) = *(bytes_pointer + 2);
            *(buffer->iterator + 2) = *(bytes_pointer + 1);
            *(buffer->iterator + 3) = *bytes_pointer;
        }

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_byte_8(MicroBuffer* buffer, Endianness endianness, const uint64_t* bytes)
{
    uint32_t data_size = sizeof(uint64_t);
    uint32_t alignment = get_alignment_offset(buffer, data_size);

    if(check_size(buffer, alignment + data_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
            *((uint64_t*)buffer->iterator) = *bytes;
        else
        {
            uint8_t* bytes_pointer = (uint8_t*)bytes;
            *buffer->iterator = *(bytes_pointer + 7);
            *(buffer->iterator + 1) = *(bytes_pointer + 6);
            *(buffer->iterator + 2) = *(bytes_pointer + 5);
            *(buffer->iterator + 3) = *(bytes_pointer + 4);
            *(buffer->iterator + 4) = *(bytes_pointer + 3);
            *(buffer->iterator + 5) = *(bytes_pointer + 2);
            *(buffer->iterator + 6) = *(bytes_pointer + 1);
            *(buffer->iterator + 7) = *bytes_pointer;
        }

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_byte_1(MicroBuffer* buffer, uint8_t* byte)
{
    uint32_t data_size = sizeof(uint8_t);
    if(check_size(buffer, data_size))
    {
        *byte = *buffer->iterator;

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_byte_2(MicroBuffer* buffer, Endianness endianness, uint16_t* bytes)
{
    uint32_t data_size = sizeof(uint16_t);
    uint32_t alignment = get_alignment_offset(buffer, data_size);

    if(check_size(buffer, alignment + data_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
            *bytes = *(uint16_t*)buffer->iterator;
        else
        {
            uint8_t* bytes_pointer = (uint8_t*)bytes;
            *bytes_pointer = *(buffer->iterator + 1);
            *(bytes_pointer + 1) = *buffer->iterator ;
        }

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_byte_4(MicroBuffer* buffer, Endianness endianness, uint32_t* bytes)
{
    uint32_t data_size = sizeof(uint32_t);
    uint32_t alignment = get_alignment_offset(buffer, data_size);

    if(check_size(buffer, alignment + data_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
            *bytes = *(uint32_t*)buffer->iterator;
        else
        {
            uint8_t* bytes_pointer = (uint8_t*)bytes;
            *bytes_pointer = *(buffer->iterator + 3);
            *(bytes_pointer + 1) = *(buffer->iterator + 2);
            *(bytes_pointer + 2) = *(buffer->iterator + 1);
            *(bytes_pointer + 3) = *buffer->iterator;
        }

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_byte_8(MicroBuffer* buffer, Endianness endianness, uint64_t* bytes)
{
    uint32_t data_size = sizeof(uint64_t);
    uint32_t alignment = get_alignment_offset(buffer, data_size);

    if(check_size(buffer, alignment + data_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
            *bytes = *(uint64_t*)buffer->iterator;
        else
        {
            uint8_t* bytes_pointer = (uint8_t*)bytes;
            *bytes_pointer = *(buffer->iterator + 7);
            *(bytes_pointer + 1) = *(buffer->iterator + 6);
            *(bytes_pointer + 2) = *(buffer->iterator + 5);
            *(bytes_pointer + 3) = *(buffer->iterator + 4);
            *(bytes_pointer + 4) = *(buffer->iterator + 3);
            *(bytes_pointer + 5) = *(buffer->iterator + 2);
            *(bytes_pointer + 6) = *(buffer->iterator + 1);
            *(bytes_pointer + 7) = *buffer->iterator;
        }

        buffer->iterator += data_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_array_byte_1(MicroBuffer* buffer, const uint8_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint8_t);
    if(check_size(buffer, size))
    {
        memcpy(buffer->iterator, array, size);

        buffer->iterator += size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_array_byte_2(MicroBuffer* buffer, Endianness endianness, const uint16_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint16_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint16_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;
        if(MACHINE_ENDIANNESS == endianness)
        {
            memcpy(buffer->iterator, array, array_size);

            buffer->iterator += array_size;
            buffer->last_data_size = data_size;
        }
        else
        {
            for(uint32_t i = 0; i < size; i++)
                serialize_byte_2(buffer, endianness, array + i);
        }
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_array_byte_4(MicroBuffer* buffer, Endianness endianness, const uint32_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint32_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint32_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
        {
            memcpy(buffer->iterator, array, array_size);
            buffer->iterator += array_size;
            buffer->last_data_size = data_size;
        }
        else
        {
            for(uint32_t i = 0; i < size; i++)
                serialize_byte_4(buffer, endianness, array + i);
        }
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_array_byte_8(MicroBuffer* buffer, Endianness endianness, const uint64_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint64_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint64_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
        {
            memcpy(buffer->iterator, array, array_size);
            buffer->iterator += array_size;
            buffer->last_data_size = data_size;
        }
        else
        {
            for(uint32_t i = 0; i < size; i++)
                serialize_byte_8(buffer, endianness, array + i);
        }
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_array_byte_1(MicroBuffer* buffer, uint8_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint8_t);
    if(check_size(buffer, size))
    {
        memcpy(array, buffer->iterator, size);

        buffer->iterator += size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_array_byte_2(MicroBuffer* buffer, Endianness endianness, uint16_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint16_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint16_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
        {
            memcpy(array, buffer->iterator, array_size);
            buffer->iterator += array_size;
            buffer->last_data_size = data_size;
        }
        else
        {
            for(uint32_t i = 0; i < size; i++)
                deserialize_byte_2(buffer, endianness, array + i);
        }
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_array_byte_4(MicroBuffer* buffer, Endianness endianness, uint32_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint32_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint32_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
        {
            memcpy(array, buffer->iterator, array_size);
            buffer->iterator += array_size;
            buffer->last_data_size = data_size;
        }
        else
        {
            for(uint32_t i = 0; i < size; i++)
                deserialize_byte_4(buffer, endianness, array + i);
        }
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_array_byte_8(MicroBuffer* buffer, Endianness endianness, uint64_t* array, uint32_t size)
{
    uint32_t data_size = sizeof(uint64_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint64_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;

        if(MACHINE_ENDIANNESS == endianness)
        {
            memcpy(array, buffer->iterator, array_size);
            buffer->iterator += array_size;
            buffer->last_data_size = data_size;
        }
        else
        {
            for(uint32_t i = 0; i < size; i++)
                deserialize_byte_8(buffer, endianness, array + i);
        }
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_inline_array_byte_1(MicroBuffer* buffer, uint8_t** array, uint32_t size)
{
    uint32_t data_size = sizeof(uint8_t);
    if(check_size(buffer, size))
    {
        *array = buffer->iterator;
        buffer->iterator += size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_inline_array_byte_2(MicroBuffer* buffer, Endianness endianness, uint16_t** array,  uint32_t size)
{
    uint32_t data_size = sizeof(uint16_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint16_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;
        *array = (uint16_t*) buffer->iterator;
        if(MACHINE_ENDIANNESS != endianness)
        {
            for(uint32_t i = 0; i < array_size; i += data_size)
            {
                uint8_t aux = buffer->iterator[i];
                buffer->iterator[i] = buffer->iterator[i + 1];
                buffer->iterator[i + 1] = aux;
            }
        }

        buffer->iterator += array_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_inline_array_byte_4(MicroBuffer* buffer, Endianness endianness, uint32_t** array, uint32_t size)
{
    uint32_t data_size = sizeof(uint32_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint32_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;
        *array = (uint32_t*) buffer->iterator;
        if(MACHINE_ENDIANNESS != endianness)
        {
            for(uint32_t i = 0; i < array_size; i += data_size)
            {
                uint8_t aux1 = buffer->iterator[i];
                uint8_t aux2 = buffer->iterator[i + 1];
                buffer->iterator[i] = buffer->iterator[i + 3];
                buffer->iterator[i + 1] = buffer->iterator[i + 2];
                buffer->iterator[i + 2] = aux2;
                buffer->iterator[i + 3] = aux1;
            }
        }

        buffer->iterator += array_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool deserialize_inline_array_byte_8(MicroBuffer* buffer, Endianness endianness, uint64_t** array, uint32_t size)
{
    uint32_t data_size = sizeof(uint64_t);
    uint32_t array_size = size * data_size;
    uint32_t alignment = get_alignment_offset(buffer, sizeof(uint64_t));

    if(check_size(buffer, alignment + array_size))
    {
        buffer->iterator += alignment;
        *array = (uint64_t*) buffer->iterator;
        if(MACHINE_ENDIANNESS != endianness)
        {
            for(uint32_t i = 0; i < array_size; i += data_size)
            {
                uint8_t aux1 = buffer->iterator[i];
                uint8_t aux2 = buffer->iterator[i + 1];
                uint8_t aux3 = buffer->iterator[i + 2];
                uint8_t aux4 = buffer->iterator[i + 3];
                buffer->iterator[i] = buffer->iterator[i + 7];
                buffer->iterator[i + 1] = buffer->iterator[i + 6];
                buffer->iterator[i + 2] = buffer->iterator[i + 5];
                buffer->iterator[i + 3] = buffer->iterator[i + 4];
                buffer->iterator[i + 4] = aux4;
                buffer->iterator[i + 5] = aux3;
                buffer->iterator[i + 6] = aux2;
                buffer->iterator[i + 7] = aux1;
            }
        }

        buffer->iterator += array_size;
        buffer->last_data_size = data_size;
        return true;
    }
    buffer->error = BUFFER_NOK;
    return false;
}

bool serialize_char(MicroBuffer* buffer, char value)
{
    return serialize_byte_1(buffer, (uint8_t*)&value);
}

bool serialize_bool(MicroBuffer* buffer, bool value)
{
    return serialize_byte_1(buffer, (uint8_t*)&value);
}

bool serialize_uint8_t(MicroBuffer* buffer, uint8_t value)
{
    return serialize_byte_1(buffer, &value);
}

bool serialize_uint16_t(MicroBuffer* buffer, uint16_t value)
{
    return serialize_byte_2(buffer, buffer->endianness, &value);
}

bool serialize_uint32_t(MicroBuffer* buffer, uint32_t value)
{
    return serialize_byte_4(buffer, buffer->endianness, &value);
}

bool serialize_uint64_t(MicroBuffer* buffer, uint64_t value)
{
    return serialize_byte_8(buffer, buffer->endianness, &value);
}

bool serialize_int16_t(MicroBuffer* buffer, int16_t value)
{
    return serialize_byte_2(buffer, buffer->endianness, (uint16_t*)&value);
}

bool serialize_int32_t(MicroBuffer* buffer, int32_t value)
{
    return serialize_byte_4(buffer, buffer->endianness, (uint32_t*)&value);
}

bool serialize_int64_t(MicroBuffer* buffer, int64_t value)
{
    return serialize_byte_8(buffer, buffer->endianness, (uint64_t*)&value);
}

bool serialize_float(MicroBuffer* buffer, float value)
{
    return serialize_byte_4(buffer, buffer->endianness, (uint32_t*)&value);
}

bool serialize_double(MicroBuffer* buffer, double value)
{
    return serialize_byte_8(buffer, buffer->endianness, (uint64_t*)&value);
}

bool deserialize_char(MicroBuffer* buffer, char* value)
{
    return deserialize_byte_1(buffer, (uint8_t*)value);
}

bool deserialize_bool(MicroBuffer* buffer, bool* value)
{
    return deserialize_byte_1(buffer, (uint8_t*)value);
}

bool deserialize_uint8_t(MicroBuffer* buffer, uint8_t* value)
{
    return deserialize_byte_1(buffer, value);
}

bool deserialize_uint16_t(MicroBuffer* buffer, uint16_t* value)
{
    return deserialize_byte_2(buffer, buffer->endianness, value);
}

bool deserialize_uint32_t(MicroBuffer* buffer, uint32_t* value)
{
    return deserialize_byte_4(buffer, buffer->endianness, value);
}

bool deserialize_uint64_t(MicroBuffer* buffer, uint64_t* value)
{
    return deserialize_byte_8(buffer, buffer->endianness, value);
}

bool deserialize_int16_t(MicroBuffer* buffer, int16_t* value)
{
    return deserialize_byte_2(buffer, buffer->endianness, (uint16_t*)value);
}

bool deserialize_int32_t(MicroBuffer* buffer, int32_t* value)
{
    return deserialize_byte_4(buffer, buffer->endianness, (uint32_t*)value);
}

bool deserialize_int64_t(MicroBuffer* buffer, int64_t* value)
{
    return deserialize_byte_8(buffer, buffer->endianness, (uint64_t*)value);
}

bool deserialize_float(MicroBuffer* buffer, float* value)
{
    return deserialize_byte_4(buffer, buffer->endianness, (uint32_t*)value);
}

bool deserialize_double(MicroBuffer* buffer, double* value)
{
    return deserialize_byte_8(buffer, buffer->endianness, (uint64_t*)value);
}

bool serialize_array_char(MicroBuffer* buffer, const char* array, uint32_t size)
{
    return serialize_array_byte_1(buffer, (uint8_t*)array, size);
}

bool serialize_array_bool(MicroBuffer* buffer, const bool* array, uint32_t size)
{
    return serialize_array_byte_1(buffer, (uint8_t*)array, size);
}

bool serialize_array_uint8_t(MicroBuffer* buffer, const uint8_t* array, uint32_t size)
{
    return serialize_array_byte_1(buffer, array, size);
}

bool serialize_array_uint16_t(MicroBuffer* buffer, const uint16_t* array, uint32_t size)
{
    return serialize_array_byte_2(buffer, buffer->endianness, array, size);
}

bool serialize_array_uint32_t(MicroBuffer* buffer, const uint32_t* array, uint32_t size)
{
    return serialize_array_byte_4(buffer, buffer->endianness, array, size);
}

bool serialize_array_uint64_t(MicroBuffer* buffer, const uint64_t* array, uint32_t size)
{
    return serialize_array_byte_8(buffer, buffer->endianness, array, size);
}

bool serialize_array_int16_t(MicroBuffer* buffer, const int16_t* array, uint32_t size)
{
    return serialize_array_byte_2(buffer, buffer->endianness, (uint16_t*)array, size);
}

bool serialize_array_int32_t(MicroBuffer* buffer, const int32_t* array, uint32_t size)
{
    return serialize_array_byte_4(buffer, buffer->endianness, (uint32_t*)array, size);
}

bool serialize_array_int64_t(MicroBuffer* buffer, const int64_t* array, uint32_t size)
{
    return serialize_array_byte_8(buffer, buffer->endianness, (uint64_t*)array, size);
}

bool serialize_array_float(MicroBuffer* buffer, const float* array, uint32_t size)
{
    return serialize_array_byte_4(buffer, buffer->endianness, (uint32_t*)array, size);
}

bool serialize_array_double(MicroBuffer* buffer, const double* array, uint32_t size)
{
    return serialize_array_byte_8(buffer, buffer->endianness, (uint64_t*)array, size);
}

bool deserialize_array_char(MicroBuffer* buffer, char* array, uint32_t size)
{
    return deserialize_array_byte_1(buffer, (uint8_t*)array, size);
}

bool deserialize_array_bool(MicroBuffer* buffer, bool* array, uint32_t size)
{
    return deserialize_array_byte_1(buffer, (uint8_t*)array, size);
}

bool deserialize_array_uint8_t(MicroBuffer* buffer, uint8_t* array, uint32_t size)
{
    return deserialize_array_byte_1(buffer, array, size);
}

bool deserialize_array_uint16_t(MicroBuffer* buffer, uint16_t* array, uint32_t size)
{
    return deserialize_array_byte_2(buffer, buffer->endianness, array, size);
}

bool deserialize_array_uint32_t(MicroBuffer* buffer, uint32_t* array, uint32_t size)
{
    return deserialize_array_byte_4(buffer, buffer->endianness, array, size);
}

bool deserialize_array_uint64_t(MicroBuffer* buffer, uint64_t* array, uint32_t size)
{
    return deserialize_array_byte_8(buffer, buffer->endianness, array, size);
}

bool deserialize_array_int16_t(MicroBuffer* buffer, int16_t* array, uint32_t size)
{
    return deserialize_array_byte_2(buffer, buffer->endianness, (uint16_t*)array, size);
}

bool deserialize_array_int32_t(MicroBuffer* buffer, int32_t* array, uint32_t size)
{
    return deserialize_array_byte_4(buffer, buffer->endianness, (uint32_t*)array, size);
}

bool deserialize_array_int64_t(MicroBuffer* buffer, int64_t* array, uint32_t size)
{
    return deserialize_array_byte_8(buffer, buffer->endianness, (uint64_t*)array, size);
}

bool deserialize_array_float(MicroBuffer* buffer, float* array, uint32_t size)
{
    return deserialize_array_byte_4(buffer, buffer->endianness, (uint32_t*)array, size);
}

bool deserialize_array_double(MicroBuffer* buffer, double* array, uint32_t size)
{
    return deserialize_array_byte_8(buffer, buffer->endianness, (uint64_t*)array, size);
}

bool deserialize_inline_array_char(MicroBuffer* buffer, char** array, uint32_t size)
{
    return deserialize_inline_array_byte_1(buffer, (uint8_t**) array, size);
}

bool deserialize_inline_array_bool(MicroBuffer* buffer, bool** array, uint32_t size)
{
    return deserialize_inline_array_byte_1(buffer, (uint8_t**)array, size);
}

bool deserialize_inline_array_uint8_t(MicroBuffer* buffer, uint8_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_1(buffer, array, size);
}

bool deserialize_inline_array_uint16_t(MicroBuffer* buffer, uint16_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_2(buffer, buffer->endianness, array, size);
}

bool deserialize_inline_array_uint32_t(MicroBuffer* buffer, uint32_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_4(buffer, buffer->endianness, array, size);
}

bool deserialize_inline_array_uint64_t(MicroBuffer* buffer, uint64_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_8(buffer, buffer->endianness, array, size);
}

bool deserialize_inline_array_int16_t(MicroBuffer* buffer, int16_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_2(buffer, buffer->endianness, (uint16_t**) array, size);
}

bool deserialize_inline_array_int32_t(MicroBuffer* buffer, int32_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_4(buffer, buffer->endianness, (uint32_t**) array, size);
}

bool deserialize_inline_array_int64_t(MicroBuffer* buffer, int64_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_8(buffer, buffer->endianness, (uint64_t**) array, size);
}

bool deserialize_inline_array_float(MicroBuffer* buffer, float** array, uint32_t size)
{
    return deserialize_inline_array_byte_4(buffer, buffer->endianness, (uint32_t**) array, size);
}

bool deserialize_inline_array_double(MicroBuffer* buffer, double** array, uint32_t size)
{
    return deserialize_inline_array_byte_8(buffer, buffer->endianness, (uint64_t**) array, size);
}

bool serialize_sequence_char(MicroBuffer* buffer, const char* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_char(buffer, array, size);
}

bool serialize_sequence_bool(MicroBuffer* buffer, const bool* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_bool(buffer, array, size);
}

bool serialize_sequence_uint8_t(MicroBuffer* buffer, const uint8_t* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_uint8_t(buffer, array, size);
}

bool serialize_sequence_uint16_t(MicroBuffer* buffer, const uint16_t* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_uint16_t(buffer, array, size);
}

bool serialize_sequence_uint32_t(MicroBuffer* buffer, const uint32_t* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_uint32_t(buffer, array, size);
}

bool serialize_sequence_uint64_t(MicroBuffer* buffer, const uint64_t* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_uint64_t(buffer, array, size);
}

bool serialize_sequence_int16_t(MicroBuffer* buffer, const int16_t* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_int16_t(buffer, array, size);
}

bool serialize_sequence_int32_t(MicroBuffer* buffer, const int32_t* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_int32_t(buffer, array, size);
}

bool serialize_sequence_int64_t(MicroBuffer* buffer, const int64_t* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_int64_t(buffer, array, size);
}

bool serialize_sequence_float(MicroBuffer* buffer, const float* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_float(buffer, array, size);
}

bool serialize_sequence_double(MicroBuffer* buffer, const double* array, uint32_t size)
{
    serialize_uint32_t(buffer, size);
    return serialize_array_double(buffer, array, size);
}

bool deserialize_sequence_char(MicroBuffer* buffer, char** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_char(buffer, array, *size);
}

bool deserialize_sequence_bool(MicroBuffer* buffer, bool** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_bool(buffer, array, *size);
}

bool deserialize_sequence_uint8_t(MicroBuffer* buffer, uint8_t** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_uint8_t(buffer, array, *size);
}

bool deserialize_sequence_uint16_t(MicroBuffer* buffer, uint16_t** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_uint16_t(buffer, array, *size);
}

bool deserialize_sequence_uint32_t(MicroBuffer* buffer, uint32_t** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_uint32_t(buffer, array, *size);
}

bool deserialize_sequence_uint64_t(MicroBuffer* buffer, uint64_t** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_uint64_t(buffer, array, *size);
}

bool deserialize_sequence_int16_t(MicroBuffer* buffer, int16_t** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_int16_t(buffer, array, *size);
}

bool deserialize_sequence_int32_t(MicroBuffer* buffer, int32_t** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_int32_t(buffer, array, *size);
}

bool deserialize_sequence_int64_t(MicroBuffer* buffer, int64_t** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_int64_t(buffer, array, *size);
}

bool deserialize_sequence_float(MicroBuffer* buffer, float** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_float(buffer, array, *size);
}

bool deserialize_sequence_double(MicroBuffer* buffer, double** array, uint32_t* size)
{
    deserialize_uint32_t(buffer, size);
    return deserialize_inline_array_double(buffer, array, *size);
}

bool serialize_endian_uint16_t(MicroBuffer* buffer, Endianness endianness, uint16_t value)
{
    return serialize_byte_2(buffer, endianness, &value);
}

bool serialize_endian_uint32_t(MicroBuffer* buffer, Endianness endianness, uint32_t value)
{
    return serialize_byte_4(buffer, endianness, &value);
}

bool serialize_endian_uint64_t(MicroBuffer* buffer, Endianness endianness, uint64_t value)
{
    return serialize_byte_8(buffer, endianness, &value);
}

bool serialize_endian_int16_t(MicroBuffer* buffer, Endianness endianness, int16_t value)
{
    return serialize_byte_2(buffer, endianness, (uint16_t*)&value);
}

bool serialize_endian_int32_t(MicroBuffer* buffer, Endianness endianness, int32_t value)
{
    return serialize_byte_4(buffer, endianness, (uint32_t*)&value);
}

bool serialize_endian_int64_t(MicroBuffer* buffer, Endianness endianness, int64_t value)
{
    return serialize_byte_8(buffer, endianness, (uint64_t*)&value);
}

bool serialize_endian_float(MicroBuffer* buffer, Endianness endianness, float value)
{
    return serialize_byte_4(buffer, endianness, (uint32_t*)&value);
}

bool serialize_endian_double(MicroBuffer* buffer, Endianness endianness, double value)
{
    return serialize_byte_8(buffer, endianness, (uint64_t*)&value);
}

bool deserialize_endian_uint16_t(MicroBuffer* buffer, Endianness endianness, uint16_t* value)
{
    return deserialize_byte_2(buffer, endianness, value);
}

bool deserialize_endian_uint32_t(MicroBuffer* buffer, Endianness endianness, uint32_t* value)
{
    return deserialize_byte_4(buffer, endianness, value);
}

bool deserialize_endian_uint64_t(MicroBuffer* buffer, Endianness endianness, uint64_t* value)
{
    return deserialize_byte_8(buffer, endianness, value);
}

bool deserialize_endian_int16_t(MicroBuffer* buffer, Endianness endianness, int16_t* value)
{
    return deserialize_byte_2(buffer, endianness, (uint16_t*)value);
}

bool deserialize_endian_int32_t(MicroBuffer* buffer, Endianness endianness, int32_t* value)
{
    return deserialize_byte_4(buffer, endianness, (uint32_t*)value);
}

bool deserialize_endian_int64_t(MicroBuffer* buffer, Endianness endianness, int64_t* value)
{
    return deserialize_byte_8(buffer, endianness, (uint64_t*)value);
}

bool deserialize_endian_float(MicroBuffer* buffer, Endianness endianness, float* value)
{
    return deserialize_byte_4(buffer, endianness, (uint32_t*)value);
}

bool deserialize_endian_double(MicroBuffer* buffer, Endianness endianness, double* value)
{
    return deserialize_byte_8(buffer, endianness, (uint64_t*)value);
}

bool serialize_endian_array_uint16_t(MicroBuffer* buffer, Endianness endianness, const uint16_t* array, uint32_t size)
{
    return serialize_array_byte_2(buffer, endianness, array, size);
}

bool serialize_endian_array_uint32_t(MicroBuffer* buffer, Endianness endianness, const uint32_t* array, uint32_t size)
{
    return serialize_array_byte_4(buffer, endianness, array, size);
}

bool serialize_endian_array_uint64_t(MicroBuffer* buffer, Endianness endianness, const uint64_t* array, uint32_t size)
{
    return serialize_array_byte_8(buffer, endianness, array, size);
}

bool serialize_endian_array_int16_t(MicroBuffer* buffer, Endianness endianness, const int16_t* array, uint32_t size)
{
    return serialize_array_byte_2(buffer, endianness, (uint16_t*)array, size);
}

bool serialize_endian_array_int32_t(MicroBuffer* buffer, Endianness endianness, const int32_t* array, uint32_t size)
{
    return serialize_array_byte_4(buffer, endianness, (uint32_t*)array, size);
}

bool serialize_endian_array_int64_t(MicroBuffer* buffer, Endianness endianness, const int64_t* array, uint32_t size)
{
    return serialize_array_byte_8(buffer, endianness, (uint64_t*)array, size);
}

bool serialize_endian_array_float(MicroBuffer* buffer, Endianness endianness, const float* array, uint32_t size)
{
    return serialize_array_byte_4(buffer, endianness, (uint32_t*)array, size);
}

bool serialize_endian_array_double(MicroBuffer* buffer, Endianness endianness, const double* array, uint32_t size)
{
    return serialize_array_byte_8(buffer, endianness, (uint64_t*)array, size);
}

bool deserialize_endian_array_uint16_t(MicroBuffer* buffer, Endianness endianness, uint16_t* array, uint32_t size)
{
    return deserialize_array_byte_2(buffer, endianness, array, size);
}

bool deserialize_endian_array_uint32_t(MicroBuffer* buffer, Endianness endianness, uint32_t* array, uint32_t size)
{
    return deserialize_array_byte_4(buffer, endianness, array, size);
}

bool deserialize_endian_array_uint64_t(MicroBuffer* buffer, Endianness endianness, uint64_t* array, uint32_t size)
{
    return deserialize_array_byte_8(buffer, endianness, array, size);
}

bool deserialize_endian_array_int16_t(MicroBuffer* buffer, Endianness endianness, int16_t* array, uint32_t size)
{
    return deserialize_array_byte_2(buffer, endianness, (uint16_t*)array, size);
}

bool deserialize_endian_array_int32_t(MicroBuffer* buffer, Endianness endianness, int32_t* array, uint32_t size)
{
    return deserialize_array_byte_4(buffer, endianness, (uint32_t*)array, size);
}

bool deserialize_endian_array_int64_t(MicroBuffer* buffer, Endianness endianness, int64_t* array, uint32_t size)
{
    return deserialize_array_byte_8(buffer, endianness, (uint64_t*)array, size);
}

bool deserialize_endian_array_float(MicroBuffer* buffer, Endianness endianness, float* array, uint32_t size)
{
    return deserialize_array_byte_4(buffer, endianness, (uint32_t*)array, size);
}

bool deserialize_endian_array_double(MicroBuffer* buffer, Endianness endianness, double* array, uint32_t size)
{
    return deserialize_array_byte_8(buffer, endianness, (uint64_t*)array, size);
}

bool deserialize_endian_inline_array_uint16_t(MicroBuffer* buffer, Endianness endianness, uint16_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_2(buffer, endianness, array, size);
}

bool deserialize_endian_inline_array_uint32_t(MicroBuffer* buffer, Endianness endianness, uint32_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_4(buffer, endianness, array, size);
}

bool deserialize_endian_inline_array_uint64_t(MicroBuffer* buffer, Endianness endianness, uint64_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_8(buffer, endianness, array, size);
}

bool deserialize_endian_inline_array_int16_t(MicroBuffer* buffer, Endianness endianness, int16_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_2(buffer, endianness, (uint16_t**) array, size);
}

bool deserialize_endian_inline_array_int32_t(MicroBuffer* buffer, Endianness endianness, int32_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_4(buffer, endianness, (uint32_t** ) array, size);
}

bool deserialize_endian_inline_array_int64_t(MicroBuffer* buffer, Endianness endianness, int64_t** array, uint32_t size)
{
    return deserialize_inline_array_byte_8(buffer, endianness, (uint64_t**) array, size);
}

bool deserialize_endian_inline_array_float(MicroBuffer* buffer, Endianness endianness, float** array,  uint32_t size)
{
    return deserialize_inline_array_byte_4(buffer, endianness, (uint32_t**) array, size);
}

bool deserialize_endian_inline_array_double(MicroBuffer* buffer, Endianness endianness, double** array, uint32_t size)
{
    return deserialize_inline_array_byte_8(buffer, endianness, (uint64_t**) array, size);
}

bool serialize_endian_sequence_uint16_t(MicroBuffer* buffer, Endianness endianness, const uint16_t* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_uint16_t(buffer, endianness, array, size);
}

bool serialize_endian_sequence_uint32_t(MicroBuffer* buffer, Endianness endianness, const uint32_t* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_uint32_t(buffer, endianness, array, size);
}

bool serialize_endian_sequence_uint64_t(MicroBuffer* buffer, Endianness endianness, const uint64_t* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_uint64_t(buffer, endianness, array, size);
}

bool serialize_endian_sequence_int16_t(MicroBuffer* buffer, Endianness endianness, const int16_t* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_int16_t(buffer, endianness, array, size);
}

bool serialize_endian_sequence_int32_t(MicroBuffer* buffer, Endianness endianness, const int32_t* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_int32_t(buffer, endianness, array, size);
}

bool serialize_endian_sequence_int64_t(MicroBuffer* buffer, Endianness endianness, const int64_t* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_int64_t(buffer, endianness, array, size);
}

bool serialize_endian_sequence_float(MicroBuffer* buffer, Endianness endianness, const float* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_float(buffer, endianness, array, size);
}

bool serialize_endian_sequence_double(MicroBuffer* buffer, Endianness endianness, const double* array, uint32_t size)
{
    serialize_endian_uint32_t(buffer, endianness, size);
    return serialize_endian_array_double(buffer, endianness, array, size);
}

bool deserialize_endian_sequence_uint16_t(MicroBuffer* buffer, Endianness endianness, uint16_t** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_uint16_t(buffer, endianness, array, *size);
}

bool deserialize_endian_sequence_uint32_t(MicroBuffer* buffer, Endianness endianness, uint32_t** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_uint32_t(buffer, endianness, array, *size);
}

bool deserialize_endian_sequence_uint64_t(MicroBuffer* buffer, Endianness endianness, uint64_t** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_uint64_t(buffer, endianness, array, *size);
}

bool deserialize_endian_sequence_int16_t(MicroBuffer* buffer, Endianness endianness, int16_t** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_int16_t(buffer, endianness, array, *size);
}

bool deserialize_endian_sequence_int32_t(MicroBuffer* buffer, Endianness endianness, int32_t** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_int32_t(buffer, endianness, array, *size);
}

bool deserialize_endian_sequence_int64_t(MicroBuffer* buffer, Endianness endianness, int64_t** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_int64_t(buffer, endianness, array, *size);
}

bool deserialize_endian_sequence_float(MicroBuffer* buffer, Endianness endianness, float** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_float(buffer, endianness, array, *size);
}

bool deserialize_endian_sequence_double(MicroBuffer* buffer, Endianness endianness, double** array, uint32_t* size)
{
    deserialize_endian_uint32_t(buffer, endianness, size);
    return deserialize_endian_inline_array_double(buffer, endianness, array, *size);
}

