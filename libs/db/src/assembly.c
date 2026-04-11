#include "dbat/db/assembly.h"
#include <stdlib.h>

void free_assembly(struct assembly_data *assembly)
{
  if (assembly->pComponents) {
    free(assembly->pComponents);
    assembly->pComponents = nullptr;
  }
  free(assembly);
}