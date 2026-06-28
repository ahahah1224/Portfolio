#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int AllCleaning(void *buffer, size_t size)
{
    uint8_t *dst = (uint8_t *)buffer;
    uint64_t src = 0;

    size_t blocks_64 = size / 8;
    for (size_t i = 0; i < blocks_64; i++) {
        *(uint64_t *)dst = src;
        dst += 8;
    }
    size_t rem = size % 8;

    if (rem >= 4) {
        *(uint32_t *)dst = *(uint32_t*)&src;
        dst += 4;
        rem -= 4;
    }

    if (rem >= 2) {
        *(uint16_t *)dst = *(uint16_t*)&src;
        dst += 2;
        rem -= 2;
    }
    if (rem > 0){*dst = *(uint8_t *)&src;}
    return 1;
}

int Overwriting(void *buffer, void *buf, size_t size)
{
    uint8_t *dst = (uint8_t *)buffer;
    const uint8_t *src = (const uint8_t *)buf;

    size_t blocks_64 = size / 8;
    for (size_t i = 0; i < blocks_64; i++) {
        *(uint64_t *)dst = *(const uint64_t *)src;
        dst += 8;
        src += 8;
    }
    size_t rem = size % 8;

    if (rem >= 4) {
        *(uint32_t *)dst = *(const uint32_t *)src;
        dst += 4;
        src += 4;
        rem -= 4;
    }

    if (rem >= 2) {
        *(uint16_t *)dst = *(const uint16_t *)src;
        dst += 2;
        src += 2;
        rem -= 2;
    }
    if (rem > 0){*dst = *src;}
    return 1;
}


bool Comparison(void *buffer, void *buf, size_t size)
{
    uint8_t *dst = (uint8_t *)buffer;
    const uint8_t *src = (const uint8_t *)buf;
	
	
    size_t blocks_64 = size / 8;
    for (size_t i = 0; i < blocks_64; i++) {
        if(*(uint64_t *)dst != *(const uint64_t *)src)return false;
        dst += 8;
        src += 8;
    }
    size_t rem = size % 8;

    if (rem >= 4) {
        if(*(uint32_t *)dst != *(const uint32_t *)src)return false;
        dst += 4;
        src += 4;
        rem -= 4;
    }

    if (rem >= 2) {
        if(*(uint16_t *)dst != *(const uint16_t *)src)return false;
        dst += 2;
        src += 2;
        rem -= 2;
    }
    if (rem > 0){ if(*dst!=*src) return false;}
    return true;
}



typedef struct {
	size_t size_b;
	uint8_t size_type;
	uint32_t count;
} info_meta0;

void *calloc_meta0(size_t size, size_t type)
{
	info_meta0 *out = malloc(size*type+sizeof(info_meta0));
	AllCleaning(out, size*type+sizeof(info_meta0));
	
	out[0] = (info_meta0){size*type, type, size};  
	
	return (void*)&out[1];
}

void *malloc_meta0(size_t size)
{
	info_meta0 *out = malloc(size + sizeof(info_meta0));
	out[0] = (info_meta0){size, 0, 0}; 
	
	return (void*)&out[1];
}

void *realloc_meta0(void *buffer, size_t size)
{
	info_meta0 *out = realloc((info_meta0*)buffer - 1, size+sizeof(info_meta0));
	
	out[0].size_b = size;
	if(out[0].size_type != 0){
		out[0].count = (size / out[0].size_type);
	}
	
	return (void*)&out[1];
}

info_meta0 *sizeof_meta0(void *buf)
{
	return ((info_meta0*)buf) - 1;
}

int free_meta0(void *buf)
{
	if(buf != NULL)
		free(((info_meta0*)buf) - 1);
	else return 0;
	return 1;
}
