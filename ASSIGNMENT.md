# C++ Software Engineer Technical Test

The goal of this test is to evaluate your ability to parse & process a large
amount of data, using the appropriate data structures and algorithms to solve a
simple challenge.

The provided [data
file](https://www.dropbox.com/s/4qseadi3lcceq3b/hn_logs.tsv.bz2?dl=0) is a TSV
file listing all queries performed on [HN Search](https://hn.algolia.com)
during a few days.

Each line contains the timestamp and the query separated by a tab.

The goal is to extract the popular (most frequent) queries that have been done
during a specific time range.

## Instructions

Build a small command line tool in C++ exposing the following features:

- Output the number of distinct queries that have been done during a specific
  time range with this interface:

```
./hnStat distinct [--from TIMESTAMP] [--to TIMESTAMP] input_file
42
```

- Output the top N popular queries (one per line) that have been done during a
  specific time range:

```
./hnStat top nb_top_queries [--from TIMESTAMP] [--to TIMESTAMP] input_file
http%3A%2F%2Fwww.getsidekick.com%2Fblog%2Fbody-language-advice 6675
http%3A%2F%2Fwebboard.yenta4.com%2Ftopic%2F568045 4652
http%3A%2F%2Fwebboard.yenta4.com%2Ftopic%2F379035%3Fsort%3D1 3100
```

Where the optional parameters:
- `--from` specify the lower boundary for the range. If not specified, the
  range doesn't have a lower bound.
- `--to` specify the upper boundary for the range. If not specified, the range
  doesn't have a upper bound.

For each command, please give the algorithmic complexity of your solution, both
in time (CPU) and space (memory).

You may only use the C and C++ standard libraries. Your application must not
depend on any database or external software (the goal being to evaluate your
ability to choose the right data structures).

The code should build straightforwardly with standard UNIX command line tools.

## Evaluation Criteria

Please push your code to a public GitHub repository or send us a zip archive.
Feel free to include a README with whatever information you deem appropriate
for us to understand your assignment or thought process. In addition, can you
tell us approximatively the time you spent on it.

We'll evaluate:

- the complexity & scalability of your algorithm;
- the quality and architecture of your code;

⏳ No time limit. Good luck! 🙂
