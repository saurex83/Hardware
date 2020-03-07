#pragma once
#include "stddef.h"
#include "stdbool.h"

//!< Возвращает указатель на контейнер 
#define container_of(ptr, type, member) \
 ((type *)(void *)((char *)(ptr) - offsetof(type, member)))
   
#define array_items(obj, type) (sizeof(obj)/sizeof(type))
//!< Перебор содержимого массива
#define for_each_type(type, obj, iter)\
   for (type *iter = obj; iter < &obj[array_items(obj, type)]; iter++)
   
#define ptr_distance(ptr1, ptr2) ((size_t)((char*)ptr1 - (char*)ptr2))
     
//http://groups.di.unipi.it/~nids/docs/longjump_try_trow_catch.html     
#include <setjmp.h>
#define TRY do { jmp_buf ex_buf__; switch( setjmp(ex_buf__) ) { case 0: while(1) {
#define CATCH(x) break; case x:
#define FINALLY break; } default: {
#define ETRY break; } } }while(0)
#define THROW(x) longjmp(ex_buf__, x)

static inline bool is_array_ptr(void* array, void* item, size_t size){
  size_t offset = ptr_distance(item, array) % size;
  return (offset == 0);                              
}

static inline size_t array_index(void* array, void* item, size_t size){
  size_t index = ptr_distance(item, array) / size;
  return index;                              
}                                