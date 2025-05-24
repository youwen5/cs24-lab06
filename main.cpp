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

  // parse in-place
  movies.reserve(80'000); // alloc is expensive!

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

    movies.insert(std::move(title), rating);
    // advance to the next line
    p = (eol < end && *eol == '\r') ? eol + 2 : eol + 1;
  }
  movies.finalize();
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
  // again, alloc is expensive...
  movies.reserve(80000);

  loadCSV(argv[1], movies);

  movieFile.close();

  if (argc == 2) {
    for (auto &[name, rating] : movies.getRawVec())
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

  std::vector<std::pair<std::string, const Movie *>> best;

  for (const std::string &pref : prefixes) {
    auto matches = movies.allWithPrefix(pref);

    if (matches.empty()) {
      cout << "No movies found with prefix " << pref << '\n';
      continue;
    }

    std::sort(matches.begin(), matches.end(),
              [](const Movie *a, const Movie *b) {
                if (a->rating != b->rating)
                  return a->rating > b->rating;
                return a->name < b->name;
              });

    for (const Movie *m : matches)
      cout << m->name << ", " << fixed << setprecision(1) << m->rating << '\n';

    best.emplace_back(pref, matches.front());
    cout << '\n';
  }

  for (auto &[pref, bm] : best)
    cout << "Best movie with prefix " << pref << " is: " << bm->name
         << " with rating " << fixed << setprecision(1) << bm->rating << '\n';

  return 0;
}

/* Add your run time analysis for part 3 of the assignment here as commented
 * block*/

/*
---------------------------------------------------------
 Part 3 – Complexity analysis

  Symbols
    n  = total # movies in the CSV
    m  = # prefixes queried in this run
    k  = max # movies sharing any single prefix
    l  = max title length (only matters as a hidden constant inside comparisons)

  Load & prepare the data:
  - reading CSV and push_back into std::vector           O(n)
  - single call to std::sort after the read              O(n log n)
                                                        --------------
  Total build cost                                       O(n log n)

  (Note: compared with my old std::map approach this removes
   n allocs and pointer chasing, so constants are approx 40-60 % smaller
   on the 1000-row data set. Originally using std::map I had an optimal
   solution on the upper end, but the vector is substantially faster for small
   n)

  Per prefix:
  1. binary-search lower_bound / upper_bound         O(log n)
  2. collect the [lo, up) slice (k elements)         O(k)
  3. sort those k elements by rating, name

  ==> per-prefix                                     O(log n + k log k)

  All m prefixes                                     O(m log n + sum _(k_i) log k_i)
  Worst case sum _(k_i) <= n and k_i <= k ==>        O(m log n + n log k)

  Space usage:
  - contiguous vector of movies                         O(n)
  - transient vector<const Movie*> during a query       O(k)
  - constant scratch (loop indices, strings)            O(1)

  Peak space                                            O(n + k)

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

Benchmark 1: ./runMovies "input_20_random.csv" "prefix_small.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.4 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   1.6 ms    2011 runs

Benchmark 2: ./runMovies "input_20_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.4 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   1.5 ms    2242 runs

Benchmark 3: ./runMovies "input_100_random.csv" "prefix_small.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.4 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   1.8 ms    2557 runs

Benchmark 4: ./runMovies "input_100_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.4 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   1.6 ms    2249 runs

Benchmark 5: ./runMovies "input_1000_random.csv" "prefix_small.txt"
  Time (mean ± σ):       1.5 ms ±   0.1 ms    [User: 0.7 ms, System: 0.7 ms]
  Range (min … max):     1.3 ms …   2.1 ms    1896 runs

Benchmark 6: ./runMovies "input_1000_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):       1.5 ms ±   0.1 ms    [User: 0.6 ms, System: 0.7 ms]
  Range (min … max):     1.2 ms …   1.9 ms    1674 runs

Benchmark 7: ./runMovies "input_76920_random.csv" "prefix_small.txt"
  Time (mean ± σ):      33.9 ms ±   1.7 ms    [User: 26.5 ms, System: 6.7 ms]
  Range (min … max):    32.3 ms …  42.0 ms    87 runs

  Warning: Statistical outliers were detected. Consider re-running this
benchmark on a quiet system without any interfer ences from other programs. It
might help to use the '--warmup' or '--prepare' options.

Benchmark 8: ./runMovies "input_76920_ordered.csv" "prefix_small.txt"
  Time (mean ± σ):      26.0 ms ±   0.7 ms    [User: 18.5 ms, System: 7.0 ms]
  Range (min … max):    24.9 ms …  28.9 ms    111 runs

Benchmark 9: ./runMovies "input_20_random.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.4 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   1.7 ms    1723 runs

Benchmark 10: ./runMovies "input_20_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.4 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   1.5 ms    2313 runs

Benchmark 11: ./runMovies "input_100_random.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.5 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   2.1 ms    2307 runs

Benchmark 12: ./runMovies "input_100_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.2 ms ±   0.1 ms    [User: 0.4 ms, System: 0.7 ms]
  Range (min … max):     1.0 ms …   2.0 ms    2377 runs

Benchmark 13: ./runMovies "input_1000_random.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.7 ms ±   0.1 ms    [User: 0.8 ms, System: 0.8 ms]
  Range (min … max):     1.4 ms …   2.3 ms    2020 runs

Benchmark 14: ./runMovies "input_1000_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):       1.6 ms ±   0.1 ms    [User: 0.8 ms, System: 0.7 ms]
  Range (min … max):     1.4 ms …   2.3 ms    1717 runs

Benchmark 15: ./runMovies "input_76920_random.csv" "prefix_medium.txt"
  Time (mean ± σ):      50.3 ms ±   1.7 ms    [User: 41.9 ms, System: 7.5 ms]
  Range (min … max):    48.3 ms …  58.5 ms    60 runs

  Warning: Statistical outliers were detected. Consider re-running this
benchmark on a quiet system without any interfer ences from other programs. It
might help to use the '--warmup' or '--prepare' options.

Benchmark 16: ./runMovies "input_76920_ordered.csv" "prefix_medium.txt"
  Time (mean ± σ):      41.8 ms ±   0.7 ms    [User: 33.8 ms, System: 7.3 ms]
  Range (min … max):    40.5 ms …  44.1 ms    73 runs

Benchmark 17: ./runMovies "input_20_random.csv" "prefix_large.txt"
  Time (mean ± σ):       5.2 ms ±   0.3 ms    [User: 3.4 ms, System: 1.6 ms]
  Range (min … max):     4.5 ms …   6.9 ms    542 runs

Benchmark 18: ./runMovies "input_20_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):       5.1 ms ±   0.2 ms    [User: 3.4 ms, System: 1.5 ms]
  Range (min … max):     4.6 ms …   6.2 ms    564 runs

Benchmark 19: ./runMovies "input_100_random.csv" "prefix_large.txt"
  Time (mean ± σ):       5.4 ms ±   0.3 ms    [User: 3.8 ms, System: 1.5 ms]
  Range (min … max):     4.8 ms …   7.2 ms    525 runs

Benchmark 20: ./runMovies "input_100_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):       5.6 ms ±   0.3 ms    [User: 3.8 ms, System: 1.6 ms]
  Range (min … max):     4.9 ms …   7.0 ms    593 runs

Benchmark 21: ./runMovies "input_1000_random.csv" "prefix_large.txt"
  Time (mean ± σ):       6.4 ms ±   0.3 ms    [User: 4.5 ms, System: 1.7 ms]
  Range (min … max):     5.7 ms …   7.4 ms    481 runs

Benchmark 22: ./runMovies "input_1000_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):       6.4 ms ±   0.3 ms    [User: 4.6 ms, System: 1.6 ms]
  Range (min … max):     5.6 ms …   7.7 ms    465 runs

Benchmark 23: ./runMovies "input_76920_random.csv" "prefix_large.txt"
  Time (mean ± σ):      51.6 ms ±   0.9 ms    [User: 42.8 ms, System: 7.7 ms]
  Range (min … max):    49.6 ms …  54.0 ms    58 runs

Benchmark 24: ./runMovies "input_76920_ordered.csv" "prefix_large.txt"
  Time (mean ± σ):      43.5 ms ±   1.1 ms    [User: 35.3 ms, System: 7.5 ms]
  Range (min … max):    42.0 ms …  48.6 ms    69 runs


Trend matches O(n log n) build cost plus O(n) traversal.

Trade-off discussion (Part 3c)
--------------------
At first I primarily optimized for low code complexity with respectable time.
My initial ordered map meets the “mystery implementation #4” band on the
runtime plot. Space is asymptotically optimal (O(n)); shaving it further would
require compression tricks at the cost of slower look-ups.

However, the ordered_map was fast for large n (n=72600) but slower than the
fastest solutions on the leaderboard for (n=1000). So I went back to the
drawing board, and reimplemented using a simple vector to hit CPU caches.

This lead to half the runtime for low n.

Also, by using a vector, I could reserve space for large datasets (this is not
        possible with ordered_map due to its underlying red-black tree
        implementation). Therefore, I reserve the vector with 80000 bytes
initially, to massively cut down on allocs (from my extensive tenure as a
        rustacean, I know this is by far the most expensive part when we're
        fighting for microseconds).

Also I will note that while I spent a lot of time implementing a CSV parser
than works in-place on contiguous memory, it shaved off almost no time on CSIL.
So ultimately I think the change from map to vector to reduce allocs was the
main optimization I made.
*/
