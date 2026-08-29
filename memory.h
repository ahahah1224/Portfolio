#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int Cleaning(void *buffer, size_t size)
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

void *Writing(void *buffer, void *buf, size_t size)
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
    if (rem > 0){
		*dst = *src;
		dst += 1;
		src += 1;
	}
    return dst;
}


bool meta_if(const void *buffer, const void *buf, size_t size)
{
    uint8_t *dst = (uint8_t *)buffer;
    const uint8_t *src = (const uint8_t *)buf;
	
	
    size_t blocks_64 = size / 8;
    for (size_t i = 0; i < blocks_64; i++) {
        if(*(uint64_t *)dst != *(const uint64_t *)src)
			return false;
        dst += 8;
        src += 8;
    }
    size_t rem = size % 8;

    if (rem >= 4) {
        if(*(uint32_t *)dst != *(const uint32_t *)src)
			return false;
        dst += 4;
        src += 4;
        rem -= 4;
    }

    if (rem >= 2) {
        if(*(uint16_t *)dst != *(const uint16_t *)src)
			return false;
        dst += 2;
        src += 2;
        rem -= 2;
    }
    if (rem > 0){ if(*dst!=*src) return false;}
    return true;
}

void meta_error(const char *report){
	printf("error: %s;\n", report);
}

void meta_free(void *buf)
{
	if(buf != NULL)
		free(((uint64_t*)buf)-1);
	else meta_error("not curect link");
}

void *meta_realloc(void *buf, size_t size)
{
	uint64_t *meta = realloc(((uint64_t*)buf)-1, size+sizeof(uint64_t));
	
	if(meta == NULL) {
		meta_error("realloc no memory allocated");
		return NULL;
	}
	
	*meta = size;
	return &meta[1];
}

void *meta_malloc(size_t size)
{
	uint64_t *meta = malloc(size + sizeof(uint64_t));
	
	if(meta == NULL) {
		meta_error("malloc no memory allocated");
		return NULL;
	}
	
	*meta = size;
	return &meta[1];
}

void *meta_calloc(size_t size, size_t type)
{
	uint64_t *meta = meta_malloc(size*type);
	Cleaning(meta-1, (size*type)+sizeof(uint64_t));
	
	if(meta == NULL) {
		meta_error("calloc no memory allocated");
		return NULL;
	}
	
	meta[-1] = size*type;
	return meta;
}
