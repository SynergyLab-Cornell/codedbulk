/***********
 * Test memory allocator
 ***********/
#include "memory-allocator.h"
#include "packet.h"
#include <iostream>
using namespace std;

#define CLASS TEST

class CLASS : public MemoryElement {
public:
  ~CLASS() {
    if(_allocated) {
      release();
    }
  }
  virtual void initialize() { std::cout << "initialize" << std::endl; }
  virtual void release () { std::cout << "release" << std::endl; }
};

int main(int argc, char* argv[]){
  MemoryAllocator<CLASS> allocator;

  std::cout << "size_t = " << sizeof(size_t) << std::endl;

  //for(int i = 0; i < 10; ++i){
    std::cout << "new" << std::endl;
    CLASS* test = allocator.New();
    std::cout << "delete" << std::endl;
    test->Delete ();
    std::cout << "new again" << std::endl;
    CLASS* test1 = allocator.New();
  //}

  return 0;
}