//
// Created by M.Yasin SAĞLAM on 23.12.2017.
//

#pragma once

/*
 * mm_alloc.h
 *
 * A clone of the interface documented in "man 3 malloc".
 */
#include <stdlib.h>
#include <stdio.h>

struct block{
    size_t size;
    int is_free;
    struct block *next;
    struct block *prev;
};

#define BLOCK_SIZE sizeof(struct block)

void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);
void mm_free(void *ptr);
struct block* find_free_block(size_t size);

