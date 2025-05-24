// Winter'24
// Instructor: Diba Mirza
// Student name: Youwen Wu
#include <algorithm>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "movies.h"
#include "utilities.h"

bool parseLine(string &line, string &movieName, double &movieRating);

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Not enough arguments provided (need at least 1 argument)." << endl;
    cerr << "Usage: " << argv[0] << " moviesFilename prefixFilename " << endl;
    exit(1);
  }

  ifstream movieFile(argv[1]);

  string line, movieName;
  double movieRating;

  Movies movies;

  while (getline(movieFile, line) && parseLine(line, movieName, movieRating))
    movies.insert(movieName, movieRating);

  movieFile.close();

  if (argc == 2) {
    for (auto &[name, rating] : movies.getRaw()) // empty prefix -> whole map
      cout << name << ", " << fixed << setprecision(1) << rating << '\n';
    return 0;
  }

  ifstream prefixFile(argv[2]);

  if (prefixFile.fail()) {
    cerr << "Could not open file " << argv[2];
    exit(1);
  }

  vector<string> prefixes;
  while (getline(prefixFile, line)) {
    if (!line.empty()) {
      prefixes.push_back(line);
    }
  }

  vector<pair<string, pair<string, double>>> best;

  for (const auto &pref : prefixes) {
    auto vec = movies.allWithPrefix(pref);

    if (vec.empty()) {
      cout << "No movies found with prefix " << pref << '\n';
      continue;
    }

    // sort by rating DESC, then name ASC
    sort(vec.begin(), vec.end(), [](auto &a, auto &b) {
      if (a.second != b.second)
        return a.second > b.second;
      return a.first < b.first;
    });

    for (auto &[name, rating] : vec)
      cout << name << ", " << fixed << setprecision(1) << rating << '\n';

    best.emplace_back(pref, vec.front()); // store for later summary
  }

  for (auto &[pref, bestPair] : best)
    cout << "Best movie with prefix " << pref << " is: " << bestPair.first
         << " with rating " << fixed << setprecision(1) << bestPair.second
         << '\n';

  return 0;
}

/* Add your run time analysis for part 3 of the assignment here as commented
 * block*/

bool parseLine(string &line, string &movieName, double &movieRating) {
  int commaIndex = line.find_last_of(",");
  movieName = line.substr(0, commaIndex);
  movieRating = stod(line.substr(commaIndex + 1));
  if (movieName[0] == '\"') {
    movieName = movieName.substr(1, movieName.length() - 2);
  }
  return true;
}
