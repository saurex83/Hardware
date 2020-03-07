#include "utest.h"
#include "stddef.h"

static int test1(void);
static int test2(void);
static int test3(void);
struct test_unit UT_BF_1 = {.name = "BF:push tx", .fun = test1};
struct test_unit UT_BF_2 = {.name = "BF:push-pop tx all", .fun = test2};
struct test_unit UT_BF_3 = {.name = "BF:mix rx-tx bufs", .fun = test3};

#include "buffer.h"
#include "frame.h"

static int test3(void){
  int test_res = 0;
  struct frame* fr_rx[10];
  struct frame* fr_tx[10];
  struct frame* content;
  
  
  if (BF_rx_busy() != 0)
    test_res = 31;

  if (BF_tx_busy() != 0)
    test_res = 32;

  if (BF_available()!=20)
    test_res = 33;
  
  for (int i = 0; i < 10; i++){
    fr_rx[i] = FR_create();
    fr_tx[i] = FR_create();
    if (!BF_push_tx(fr_tx[i]))
      test_res = 1;
    if (!BF_push_rx(fr_rx[i]))
      test_res = 2;    
  } 
 
  if (BF_rx_busy() != 10)
    test_res = 21;

  if (BF_tx_busy() != 10)
    test_res = 22;

  if (BF_available()!=0)
    test_res = 23;
  
  void *cursor = BF_cursor_tx();
  for (int i = 0; i < 10; i++){
      content = BF_content(cursor);
      if (content != fr_tx[9-i])
        test_res = 3;
      cursor = BF_cursor_next(cursor);
  }  

  cursor = BF_cursor_rx();
  for (int i = 0; i < 10; i++){
      content = BF_content(cursor);
      if (content != fr_rx[9-i])
        test_res = 4;
      cursor = BF_cursor_next(cursor);
  }    
  
  void *del_cursor;
  cursor = BF_cursor_tx();
  for (int i = 0; i < 10; i++){
      if (!FR_delete(fr_tx[i]))
        test_res = 5;
      del_cursor = cursor;
      cursor = BF_cursor_next(cursor);
      if (!BF_remove(del_cursor))
        test_res = 6;
  }  


  cursor = BF_cursor_rx();
  for (int i = 0; i < 10; i++){
      if (!FR_delete(fr_rx[i]))
        test_res = 7;
      del_cursor = cursor;
      cursor = BF_cursor_next(cursor);
      if (!BF_remove(del_cursor))
        test_res = 8;
  }
  
  cursor = BF_cursor_tx();
  if (cursor)
      return 9; 

  cursor = BF_cursor_rx();
  if (cursor)
      return 9;
  
 return test_res;
}

static int test2(void){
  int test_res = 0;
  
  struct frame* fr[20];
  struct frame* content;
  
  for (int i = 0; i < 20; i++){
    fr[i] = FR_create();
    if (!BF_push_tx(fr[i]))
      test_res = 1;
  }
  
  void *cursor = BF_cursor_tx();
  for (int i = 0; i <20; i++){
      content = BF_content(cursor);
      if (content != fr[19-i])
        test_res = 2;
      cursor = BF_cursor_next(cursor);
  }
  
  void *del_cursor;
  cursor = BF_cursor_tx();
  for (int i = 0; i < 20; i++){
      if (!FR_delete(fr[i]))
        test_res = 3;
      del_cursor = cursor;
      cursor = BF_cursor_next(cursor);
      if (!BF_remove(del_cursor))
        test_res = 4;
  }
  
  cursor = BF_cursor_tx();
  if (cursor)
      test_res =  5; 
  
  return test_res;
}

static int test1(void){
  int test_res = 0;
  struct frame *fr = FR_create();
  
  if (!BF_push_tx(fr))
    test_res = 1;

  void *cursor = BF_cursor_tx();
  if (!cursor)
    test_res = 2;
  
  struct frame *n_fr = BF_content(cursor);
  if (fr != n_fr)
    test_res = 3;
  
  struct frame *fr2 = FR_create();
  if (!BF_push_tx(fr2))
    test_res = 4;
  
  cursor = BF_cursor_tx();
  if (!cursor)
    test_res = 5;
  
  cursor = BF_cursor_next(cursor);  
  if (!cursor)
    test_res = 6;
    
  cursor = BF_cursor_tx();
  if (!BF_remove(cursor))
      return 8;
  
  cursor = BF_cursor_tx();
  if (!BF_remove(cursor))
      return 9;

  cursor = BF_cursor_tx();
  if (cursor)
      return 10;  
  
  if (!FR_delete(fr))
    return 11;

  if (!FR_delete(fr2))
    return 12;
  
  return test_res;
}