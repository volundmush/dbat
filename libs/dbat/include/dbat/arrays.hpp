#pragma once
#include <algorithm>
#include <cstddef>

/* Resizes an array made with new T[old_n](): the first min(old_n, new_n) elements are kept and any new ones are
 * value-initialized. p may be null when old_n is 0. */
template <typename T> void resize_array(T *&p, size_t old_n, size_t new_n)
{
  T *resized = new T[new_n]();
  std::copy_n(p, std::min(old_n, new_n), resized);
  delete[] p;
  p = resized;
}
