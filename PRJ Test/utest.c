#include "utest.h"
#include "macros.h"
#include "stdio.h"
#include "action_manager.h"

struct test_unit* TEST_FUN[] = TEST_FUN_DEF;

static int passed = 0;
static int common_count = 0;

static inline void run_unit_fun(struct test_unit* unit){
  if (!unit->fun){
  printf("Test function not assigned!!!");
  while(true);
  }
  int res;
  common_count++;
  res = unit->fun();
  
  printf("TESTING:\t%-32s", unit->name);
  if (res == 0){
    printf("\tPASSED\r\n");
    passed++;
  }
  else
    printf("\tERR = %d. FAILED <--------------\r\n", res);           
};

static void show_grating(void){
  printf("Begin unit test\r\n");
}

static void show_statistic(void){
  printf("\r\n##########################################\r\n");
  printf("Test runned: \t%d\r\n", common_count);
  printf("Test passed: \t%d\r\n", passed);
  if (common_count == passed)
    printf("Succes!\r\n");
  else
    printf("Failed!\r\n");
}

void run_tests(void){
  show_grating();
  struct test_unit* unit;
  for (int i = 0; i < sizeof(TEST_FUN)/sizeof(struct test_unit*); i++){
    unit = TEST_FUN[i];
    AM_SW_Init();
    run_unit_fun(unit);
  };
  show_statistic();  
}