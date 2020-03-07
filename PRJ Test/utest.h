#pragma once
#include "modules_test_list.h"
#define ASSERT_NONBLOCK
#include "stdbool.h"

/**
@brief Главная структура теста
*/

struct test_unit{
  char name[32];
  int (*fun)(void);
};

void run_tests(void);