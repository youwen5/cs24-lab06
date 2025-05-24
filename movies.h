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

private:
  std::map<std::string, double> byName; // key-sorted container
};
