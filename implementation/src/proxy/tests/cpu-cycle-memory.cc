/***********
 * Test how may CUP cycles will be used in memory intense applications
 ***********/
#include <iostream>
using namespace std;

class TESTCLASS {
  int state;
  double data;
  char* long_data;
};

int main(int argc, char* argv[]){
  int k = 0;
  // pause the main thread here
  for(;;){
    TESTCLASS* p = new TESTCLASS();
    ++k;
    delete p;
  }

  return 0;
}