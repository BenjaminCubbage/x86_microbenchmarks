# x86 Microbenchmarks

Microbenchmarks x86 instructions. This was made for GCC, I'm not sure if it will compile elsewhere.

This program accurately guages, for example, that my CoffeeLake processor has a maximum throughput of 4 independent ADDs/cycle, and that a simple LEA has a latency of 1 cycle. I don't know why I built this. For fun, I think?

rdtsc is used as a stopwatch, with rdtsc/lfence overhead accounted for. Results are normalized to an expected 1 cycle/dependent ADD.

