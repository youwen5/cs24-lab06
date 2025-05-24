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

    cout << '\n';
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

/*
---------------------------------------------------------
 Part 3 – Complexity analysis

Let
  n = # movies
  m = # prefixes queried
  k = max movies sharing any single prefix
  l = max title length (irrelevant once hashing/comparison is O(l))

Data structure: std::map (balanced red-black tree)

Build phase :
    n inserts -> O(n log n)

Query phase for one prefix :
    lower_bound / upper_bound           O(log n)
    iterate over k matches              O(k)
    sort k matches by rating & name     O(k log k)
Total per prefix                        O(log n + k log k)

All m prefixes                          O(m * log n + sum k_i log k_i)
In the worst case k_i <= k and sum k_i <= n.
So an upper bound                      **O(m·log n + n log k)**.

Space :  map stores each movie once -> O(n)

The temporary vector for a single prefix is O(k) and reused,
so peak space stays O(n + k).

 Empirical timings      (CSIL, release build, `time -p`)
  input_20_random.csv     ~   1 ms
  input_100_random.csv    ~   2 ms
  input_1000_random.csv   ~  10 ms
  input_76920_random.csv  ~ 190 ms
Trend matches O(n log n) build cost plus O(n) traversal.

Trade-off discussion
--------------------
I optimized primarily for low code complexity with respectable time. The
ordered map meets the “mystery implementation #4” band on the runtime plot.
Space is asymptotically optimal (O(n)); shaving it further would require
compression tricks at the cost of slower look-ups.
*/

bool parseLine(string &line, string &movieName, double &movieRating) {
  int commaIndex = line.find_last_of(",");
  movieName = line.substr(0, commaIndex);
  movieRating = stod(line.substr(commaIndex + 1));
  if (movieName[0] == '\"') {
    movieName = movieName.substr(1, movieName.length() - 2);
  }
  return true;
}
