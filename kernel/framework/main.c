#include <kernel.h>
#include <klib.h>

int main() {
  printf("b0\n");
  iset(false);

  ioe_init();
  printf("b1\n");

  cte_init(os->trap);
  printf("b2\n");

  os->init();
  DEBUG_PRINTF("b3");

  mpe_init(os->run);
  return 1;
}
