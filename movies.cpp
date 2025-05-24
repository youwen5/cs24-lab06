#include "movies.h"

std::vector<const Movie *> Movies::allWithPrefix(const std::string &p) const {
  // lower bound for the prefix
  auto lo = std::lower_bound(data.begin(), data.end(), p,
                             [](const Movie &m, const std::string &pref) {
                               return m.name.compare(0, pref.size(), pref) < 0;
                             });

  // exclusive upper bound – append char 255 (highest char) to prefix
  std::string hi = p;
  hi.push_back(char(0xFF));

  auto up = std::lower_bound(
      lo, data.end(), hi,
      [](const Movie &m, const std::string &pref) { return m.name < pref; });

  std::vector<const Movie *> out;
  out.reserve(std::distance(lo, up));
  for (auto it = lo; it != up; ++it)
    out.push_back(&*it);
  return out;
}
