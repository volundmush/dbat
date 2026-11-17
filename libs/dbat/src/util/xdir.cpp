/*
 * Directory listings, on io::list_directory(): see dbat/xdir.hpp.
 */
#include <algorithm>

#include "dbat/xdir.hpp"
#include "dbat/io/paths.hpp"

int xdir_scan(const char *dir_name, struct xap_dir *xapdirp)
{
  std::optional<std::vector<std::string>> names = io::list_directory(dir_name);

  xapdirp->current = 0;
  xapdirp->names.clear();
  if (!names) {
    xapdirp->total = -1;
    return -1;
  }
  /* as scandir() listed it, with "." and "..", which sort first */
  xapdirp->names = {".", ".."};
  xapdirp->names.insert(xapdirp->names.end(), names->begin(), names->end());
  std::ranges::sort(xapdirp->names);
  xapdirp->total = static_cast<int>(xapdirp->names.size());
  return xapdirp->total;
}

const char *xdir_get_name(struct xap_dir *xd, int i)
{
  return xd->names[static_cast<size_t>(i)].c_str();
}

void xdir_cleanup(struct xap_dir *xd)
{
  xd->names.clear();
  xd->total = 0;
}

bool insure_directory(const char *path)
{
  return io::ensure_directory(path);
}
