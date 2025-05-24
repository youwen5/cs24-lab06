#include "movies.h"

void Movies::insert(const std::string &name, double rating) {
  byName.insert({name, rating}); // duplicates?  keep the first
}

std::map<std::string, double> &Movies::getRaw() { return byName; }

auto Movies::allWithPrefix(const std::string &p) const
    -> std::vector<std::pair<std::string, double>> {
  auto lo = byName.lower_bound(p);

  // bump last character to the next possible char to form an exclusive upper
  // bound
  std::string hi = p;
  hi.back()++; // safe because names are ASCII only
  auto up = byName.lower_bound(hi);

  std::vector<std::pair<std::string, double>> out;
  for (auto it = lo; it != up; ++it)
    out.push_back(*it);
  return out;
}
