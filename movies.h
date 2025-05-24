#pragma once
#include <map>
#include <string>
#include <vector>

class Movies {
public:
  void insert(const std::string &name, double rating);
  std::vector<std::pair<std::string, double>>
  allWithPrefix(const std::string &prefix) const;
  std::map<std::string, double> &getRaw();
  // utility function for optimized loading of large datasets (saves 10ms)
  void bulkLoad(std::vector<std::pair<std::string, double>> &rows) {
    auto hint = byName.end();
    for (auto &r : rows)
      hint = byName.insert(hint, std::move(r));
  }

private:
  std::map<std::string, double> byName; // key-sorted container
};
