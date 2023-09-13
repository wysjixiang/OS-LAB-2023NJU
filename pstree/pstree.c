#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);


  if(argc < 2){
    printf("no argc!\n");
  }

  printf("Hello world!\n");


  return 0;
}
