/***********
 * Test how to drain CUP cycles
 ***********/
#include <iostream>
using namespace std;

int main(int argc, char* argv[]){
  int k = 0;
  // pause the main thread here
  for(;;++k){}

  return 0;
}