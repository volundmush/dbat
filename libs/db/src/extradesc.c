#include "dbat/db/extradesc.h"
#include <stdlib.h>

void free_extra_desc(struct extra_descr_data *extra_desc)
{
  if (extra_desc->description) {
    free(extra_desc->description);
  }
  if (extra_desc->keyword) {
    free(extra_desc->keyword);
  }
  free(extra_desc);
}

void free_extra_descs(struct extra_descr_data **extra_descs)
{
  struct extra_descr_data *next_desc = *extra_descs;
    while (next_desc) {
      struct extra_descr_data *temp = next_desc;
      next_desc = next_desc->next;
      free_extra_desc(temp);
    }
    *extra_descs = nullptr;
}