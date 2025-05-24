#include "movies.h"

void Movies::insert(const std::string &name, double rating) {
  byName.insert({name, rating}); // duplicates?  keep the first
}

std::map<std::string, double> &Movies::getRaw() { return byName; }

auto Movies::allWithPrefix(const std::string &p) const
    -> std::vector<std::pair<std::string, double>> {
  auto lo = byName.lower_bound(p);

  std::string hi(p);
  hi.push_back('\xFF'); // strictly greater than any legal ASCII
  auto up = byName.lower_bound(hi);

  std::vector<std::pair<std::string, double>> out;
  out.reserve(std::distance(lo, up)); // saves one alloc in the loop
  for (auto it = lo; it != up; ++it)
    out.push_back(*it);
  return out;
}
