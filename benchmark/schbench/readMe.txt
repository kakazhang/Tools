The scheduler wakeup latency distribution that schbench prints at the end of the benchmark run is one of its distinguishing features, and was one of the main rationales for creating it. Mason continued: "The focus on p99 latencies instead of average latencies is the most important part. For us, lots of problems only show up when you start looking at the long tail in the latency graphs." It's also a true micro-benchmark, including only the bare minimum code required to simulate Facebook's workloads while ensuring the scheduler is the slowest part of the code path.

Publishing this benchmark has provided a common tool for discussing Facebook's workloads with upstream developers, and non-Facebook engineers are now using it to test their scheduler changes, which Mason is very happy with: "I'm really grateful when I see people using schbench to help validate new patches going in."

使用方法:
  -m (--message-threads): number of message threads (def: 2)
  -t (--threads): worker threads per message thread (def: 16)
  -r (--runtime): How long to run before exiting (seconds, def: 30)
  -s (--sleeptime): Message thread latency (usec, def: 10000
  -c (--cputime): How long to think during loop (usec, def: 10000
  -a (--auto): grow thread count until latencies hurt (def: off)
  -p (--pipe): transfer size bytes to simulate a pipe test (def: 0)
  -R (--rps): requests per second mode (count, def: 0)

