// Spring'25
// Instructor: Diba Mirza
// Student name: Youwen Wu
#include <algorithm>
#include <charconv>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <string>
#include <vector>
using namespace std;

#include "movies.h"

// optimized CSV reader, saves 20ms overall, by making hardcoded assumptions
// about the csv
static void loadCSV(const std::string &path, Movies &movies) {
  // read entire file into memory
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f)
    throw std::runtime_error("cannot open " + path);

  const std::size_t len = static_cast<std::size_t>(f.tellg());
  std::string buffer(len, '\0');
  f.seekg(0, std::ios::beg);
  f.read(buffer.data(), len);

  // parse in-place without alloc
  std::vector<std::pair<std::string, double>> rows;
  rows.reserve(70'000); // alloc is expensive!

  const char *p = buffer.data();
  const char *end = p + buffer.size();

  while (p < end) {
    const char *line = p;
    const char *lastComma = nullptr;

    // single scan: remember where the last comma was
    const char *cur = line;
    while (cur < end && *cur != '\n' && *cur != '\r') {
      if (*cur == ',')
        lastComma = cur;
      ++cur;
    }
    const char *eol = cur; // cur now points at '\n', '\r', or end

    if (!lastComma)
      throw std::runtime_error("malformed csv line (no comma found)");

    // title (strip surrounding quotes if they exist)
    const char *nameBeg = (*line == '"') ? line + 1 : line;
    const char *nameEnd =
        (*line == '"' && lastComma > line) ? lastComma - 1 : lastComma;
    std::string title(nameBeg, nameEnd);

    // rating – skip any spaces after the comma
    const char *numBeg = lastComma + 1;
    while (numBeg < eol && *numBeg == ' ')
      ++numBeg;

    double rating{};
    std::from_chars(numBeg, eol, rating);

    rows.emplace_back(std::move(title), rating);

    // advance to the next line
    p = (eol < end && *eol == '\r') ? eol + 2 : eol + 1;
  }

  std::sort(rows.begin(), rows.end(),
            [](auto &a, auto &b) { return a.first < b.first; });

  movies.bulkLoad(rows);
}

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

  loadCSV(argv[1], movies);

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

  bool state = true;
  bool lastWasNotFound = false;

  for (const auto &pref : prefixes) {
    auto vec = movies.allWithPrefix(pref);

    if (vec.empty()) {
      cout << "No movies found with prefix " << pref << '\n';
      lastWasNotFound = true;
      continue;
    } else {
      lastWasNotFound = false;
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
    if (!lastWasNotFound) {
      cout << '\n';
    }
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
So an upper bound                      $O(m·log n + n log k)$.

Space :  map stores each movie once -> O(n)

The temporary vector for a single prefix is O(k) and reused,
so peak space stays O(n + k).

--- BENCHMARKS ---

NOTE: the reflection on tradeoffs are below this benchmarks section.

 Empirical timings, personal machine.
╭─────────────────┬─────────────────────╮
│ name            │ NixOS               │
│ os_version      │ 25.05               │
│ long_os_version │ Linux (NixOS 25.05) │
│ kernel_version  │ 6.14.6-zen1         │
│ hostname        │ adrastea            │
│ uptime          │ 3hr 53min 28sec     │
│ boot_time       │ 3 hours ago         │
╰─────────────────┴─────────────────────╯

BENCHMARKS --- ran for each combination of files, with `hyperfine`.

Summary
  ./runMovies "input_20_random.csv" "prefix_small.txt" ran
    1.01 ± 0.26 times faster than ./runMovies "input_20_random.csv" "prefix_medium.txt"
    1.02 ± 0.31 times faster than ./runMovies "input_20_ordered.csv" "prefix_small.txt"
    1.03 ± 0.27 times faster than ./runMovies "input_100_ordered.csv" "prefix_small.txt"
    1.04 ± 0.27 times faster than ./runMovies "input_100_ordered.csv" "prefix_medium.txt"
    1.06 ± 0.32 times faster than ./runMovies "input_20_ordered.csv" "prefix_medium.txt"
    1.08 ± 0.30 times faster than ./runMovies "input_100_random.csv" "prefix_medium.txt"
    1.08 ± 0.33 times faster than ./runMovies "input_100_random.csv" "prefix_small.txt"
    1.28 ± 0.31 times faster than ./runMovies "input_1000_ordered.csv" "prefix_small.txt"
    1.38 ± 0.36 times faster than ./runMovies "input_1000_random.csv" "prefix_small.txt"
    1.50 ± 0.34 times faster than ./runMovies "input_1000_ordered.csv" "prefix_medium.txt"
    1.59 ± 0.39 times faster than ./runMovies "input_1000_random.csv" "prefix_medium.txt"
    4.64 ± 0.92 times faster than ./runMovies "input_20_ordered.csv" "prefix_large.txt"
    4.72 ± 0.94 times faster than ./runMovies "input_20_random.csv" "prefix_large.txt"
    4.87 ± 0.94 times faster than ./runMovies "input_100_random.csv" "prefix_large.txt"
    4.98 ± 1.00 times faster than ./runMovies "input_100_ordered.csv" "prefix_large.txt"
    5.99 ± 1.15 times faster than ./runMovies "input_1000_random.csv" "prefix_large.txt"
    6.02 ± 1.19 times faster than ./runMovies "input_1000_ordered.csv" "prefix_large.txt"
   30.12 ± 5.73 times faster than ./runMovies "input_76920_ordered.csv" "prefix_small.txt"
   37.06 ± 7.48 times faster than ./runMovies "input_76920_random.csv" "prefix_small.txt"
   47.89 ± 8.91 times faster than ./runMovies "input_76920_ordered.csv" "prefix_medium.txt"
   49.93 ± 9.35 times faster than ./runMovies "input_76920_ordered.csv" "prefix_large.txt"
   56.04 ± 10.83 times faster than ./runMovies "input_76920_random.csv" "prefix_medium.txt"
   56.58 ± 10.52 times faster than ./runMovies "input_76920_random.csv" "prefix_large.txt"

Benchmark 1: ./runMovies "input_20_random.csv" "prefix_small.txt"
  Time (mean ± σ):       1.1 ms ±   0.2 ms    [User: 0.5 ms, System: 0.8 ms]
  Range (min … max):     0.7 ms …   2.0 ms    921 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 2: ./runMovies "input_20_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):       1.2 ms ±   0.3 ms    [User: 0.6 ms, System: 0.8 ms]
  Range (min … max):     0.6 ms …   2.8 ms    930 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 3: ./runMovies "input_100_random.csv" "prefix_small.txt"
  Time (mean ± σ):       1.2 ms ±   0.3 ms    [User: 0.6 ms, System: 0.8 ms]
  Range (min … max):     0.7 ms …   3.0 ms    997 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 4: ./runMovies "input_100_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):       1.2 ms ±   0.2 ms    [User: 0.5 ms, System: 0.8 ms]
  Range (min … max):     0.6 ms …   2.0 ms    966 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 5: ./runMovies "input_1000_random.csv" "prefix_small.txt"
  Time (mean ± σ):       1.6 ms ±   0.3 ms    [User: 0.8 ms, System: 0.8 ms]
  Range (min … max):     1.0 ms …   3.1 ms    806 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 6: ./runMovies "input_1000_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):       1.5 ms ±   0.2 ms    [User: 0.8 ms, System: 0.8 ms]
  Range (min … max):     0.9 ms …   2.8 ms    935 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 7: ./runMovies "input_76920_random.csv" "prefix_small.txt"
  Time (mean ± σ):      42.5 ms ±   3.4 ms    [User: 32.2 ms, System: 9.5 ms]
  Range (min … max):    38.9 ms …  60.2 ms    65 runs
 
  Warning: Statistical outliers were detected. Consider re-running this benchmark on a quiet system without any interfer
ences from other programs. It might help to use the '--warmup' or '--prepare' options.
 
Benchmark 8: ./runMovies "input_76920_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):      34.5 ms ±   1.5 ms    [User: 24.1 ms, System: 9.8 ms]
  Range (min … max):    32.8 ms …  41.5 ms    83 runs
 
Benchmark 9: ./runMovies "input_20_random.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.2 ms    [User: 0.5 ms, System: 0.8 ms]
  Range (min … max):     0.6 ms …   1.9 ms    854 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 10: ./runMovies "input_20_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.3 ms    [User: 0.6 ms, System: 0.8 ms]
  Range (min … max):     0.7 ms …   3.4 ms    941 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
  Warning: Statistical outliers were detected. Consider re-running this benchmark on a quiet system without any interfer
ences from other programs. It might help to use the '--warmup' or '--prepare' options.
 
Benchmark 11: ./runMovies "input_100_random.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.3 ms    [User: 0.6 ms, System: 0.8 ms]
  Range (min … max):     0.7 ms …   2.5 ms    1084 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 12: ./runMovies "input_100_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.2 ms    [User: 0.6 ms, System: 0.8 ms]
  Range (min … max):     0.7 ms …   2.9 ms    717 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 13: ./runMovies "input_1000_random.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.8 ms ±   0.3 ms    [User: 1.0 ms, System: 0.9 ms]
  Range (min … max):     1.2 ms …   3.3 ms    757 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 14: ./runMovies "input_1000_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.7 ms ±   0.2 ms    [User: 1.0 ms, System: 0.8 ms]
  Range (min … max):     1.2 ms …   2.7 ms    786 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 15: ./runMovies "input_76920_random.csv" "prefix_medium.txt"
  Time (mean ± σ):      64.2 ms ±   3.6 ms    [User: 52.6 ms, System: 10.5 ms]
  Range (min … max):    60.2 ms …  78.0 ms    44 runs
 
Benchmark 16: ./runMovies "input_76920_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):      54.9 ms ±   1.0 ms    [User: 43.9 ms, System: 10.0 ms]
  Range (min … max):    52.6 ms …  57.0 ms    52 runs
 
Benchmark 17: ./runMovies "input_20_random.csv" "prefix_large.txt"
  Time (mean ± σ):       5.4 ms ±   0.4 ms    [User: 3.6 ms, System: 1.7 ms]
  Range (min … max):     4.6 ms …   7.3 ms    395 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 18: ./runMovies "input_20_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):       5.3 ms ±   0.4 ms    [User: 3.6 ms, System: 1.7 ms]
  Range (min … max):     4.5 ms …   7.2 ms    409 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 19: ./runMovies "input_100_random.csv" "prefix_large.txt"
  Time (mean ± σ):       5.6 ms ±   0.3 ms    [User: 4.0 ms, System: 1.6 ms]
  Range (min … max):     4.8 ms …   6.8 ms    357 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 20: ./runMovies "input_100_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):       5.7 ms ±   0.4 ms    [User: 3.9 ms, System: 1.7 ms]
  Range (min … max):     4.8 ms …   8.2 ms    381 runs
 
  Warning: Command took less than 5 ms to complete. Note that the results might be inaccurate because hyperfine can not 
calibrate the shell startup time much more precise than this limit. You can try to use the `-N`/`--shell=none` option to
 disable the shell completely.
 
Benchmark 21: ./runMovies "input_1000_random.csv" "prefix_large.txt"
  Time (mean ± σ):       6.9 ms ±   0.3 ms    [User: 5.0 ms, System: 1.8 ms]
  Range (min … max):     6.1 ms …   7.9 ms    328 runs
 
Benchmark 22: ./runMovies "input_1000_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):       6.9 ms ±   0.5 ms    [User: 5.0 ms, System: 1.8 ms]
  Range (min … max):     6.1 ms …   8.9 ms    341 runs
 
Benchmark 23: ./runMovies "input_76920_random.csv" "prefix_large.txt"
  Time (mean ± σ):      64.8 ms ±   1.1 ms    [User: 53.4 ms, System: 10.4 ms]
  Range (min … max):    62.9 ms …  67.1 ms    45 runs
 
Benchmark 24: ./runMovies "input_76920_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):      57.2 ms ±   1.6 ms    [User: 45.1 ms, System: 11.2 ms]
  Range (min … max):    55.4 ms …  63.1 ms    51 runs


Trend matches O(n log n) build cost plus O(n) traversal.

Trade-off discussion (Part 3c)
--------------------
I optimized primarily for low code complexity with respectable time. The
ordered map meets the “mystery implementation #4” band on the runtime plot.
Space is asymptotically optimal (O(n)); shaving it further would require
compression tricks at the cost of slower look-ups.
*/
