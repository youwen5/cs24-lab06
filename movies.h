#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <vector>

struct Movie {
  std::string name;
  double rating;
};

class Movies {
public:
  void insert(const std::string &name, double rating) {
    data.emplace_back(Movie{name, rating});
  }

  // call once, after all inserts
  void finalize() {
    std::sort(data.begin(), data.end(),
              [](const Movie &a, const Movie &b) { return a.name < b.name; });
  }

  // return *views* of matching movies (no extra copies)
  std::vector<const Movie *> allWithPrefix(const std::string &p) const;
  std::vector<Movie> getRawVec() { return data; };
  void reserve(int bytes) { data.reserve(bytes); }

private:
  std::vector<Movie> data;
};
