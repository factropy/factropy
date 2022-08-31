#pragma once

// A simple thread-local pool allocator. See localvec/localset/localmap

void* lmalloc(size_t bytes);
void* lrealloc(void* ptr, size_t bytes);
void lfree(void* ptr);
void lsanity();
void linfo();
