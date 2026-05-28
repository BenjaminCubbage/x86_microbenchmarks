# x86 Microbenchmarks

Microbenchmarks x86 instructions. This was made for GCC, I'm not sure if it will compile elsewhere.

This program accurately guages, for example, that my CoffeeLake processor has a maximum throughput of 4 independent ADDs/cycle, and that a simple LEA has a latency of 1 cycle. I don't know why I built this. For fun, I think?

rdtsc is used as a stopwatch, with rdtsc/lfence overhead accounted for. Results are normalized to an expected 1 cycle/dependent ADD.

## Example Output:

### ADDL
```
dependent ticks/add:              0.570312
independent ticks/add (2 indep.): 0.289062
independent ticks/add (4 indep.): 0.152344
independent ticks/add (8 indep.): 0.148438

dependent cycles/add:                   1
est. independent cycles/add (2 indep.): 0.506849
est. independent cycles/add (4 indep.): 0.267123
est. independent cycles/add (8 indep.): 0.260274
```

### IMULL
```
dependent ticks/mul:              1.70703
independent ticks/mul (2 indep.): 0.855469
independent ticks/mul (4 indep.): 0.574219
independent ticks/mul (8 indep.): 0.574219

dependent cycles/mul:                   2.99315
est. independent cycles/mul (2 indep.): 1.5
est. independent cycles/mul (4 indep.): 1.00685
est. independent cycles/mul (8 indep.): 1.00685
```

### LEAL (Simple base + displacement)
```
dependent ticks/lea:              0.570312
independent ticks/lea (2 indep.): 0.289062
independent ticks/lea (4 indep.): 0.285156
independent ticks/lea (8 indep.): 0.285156

dependent cycles/lea:                   1
est. independent cycles/lea (2 indep.): 0.506849
est. independent cycles/lea (4 indep.): 0.5
est. independent cycles/lea (8 indep.): 0.5
```
