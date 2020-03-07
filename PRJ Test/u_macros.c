#include "utest.h"
static int test1(void);
static int test2(void);
struct test_unit UT_MACR_1 = {.name = "MACROS: EXCEPTIOS", .fun = test1};
struct test_unit UT_MACR_2 = {.name = "MACROS: EXCEPTIOS", .fun = test2};


#include "stdint.h"
#include "macros.h"
#include "stdbool.h"

static int test2(void){
  bool catched = false;
  bool finalyed = false;
  bool noexe = true;
  
  TRY{
    THROW(2);
    noexe = false;
  }
  CATCH(1){
    catched = true;
  }
  FINALLY{
    finalyed = true;
  }
  ETRY;  
  
  if (!noexe)
    return 1;

  if (catched)
    return 2;  

  if (!finalyed)
    return 3;
    
  return 0;
}


static int test1(void){
  bool catched = false;
  bool finalyed = false;
  bool noexe = true;
  
  TRY{
    THROW(1);
    noexe = false;
  }
  CATCH(1){
    catched = true;
  }
  FINALLY{
    finalyed = true;
  }
  ETRY;  
  
  if (!noexe)
    return 1;

  if (!catched)
    return 2;  

  if (!finalyed)
    return 3;
    
  return 0;
}


