 perf record -g ./build/hnStat top 10 hn_logs.tsv
 perf script > out.perf
 stackcollapse-perf.pl out.perf > out.folded
 flamegraph.pl out.folded > out.svg
