Matthew Dupont and Louis Finney

Exercise 2

Multi-process work distribution

1)

Our main code is included as `polynomial.c` and `polynomial.h`. The usage of these functions can be seen as here:

We ran these functions on term sizes of 50,000, 75,000, and 100,000, for 1.. 16 processors using a bash script in `evaluate_totals.sh`, which produced a csv row for each combination that included the max time.  These were ran for each of `sequential` procedure, as well as `round robin` and `chunked` procedures, all written to their own csv output in `results/`
These results were then aggregated and analyzed in R (`mp_workload.R`), where speedup and maxtime were graphed, as shown below:

![](images\ChunkMaxTime.png)

![](images\RoundRobinMaxTime.png)

![](images\ChunkSpeedUp.png)

![](images\RoundRobinSpeedup.png)

We note how the `roundRobin` method produces a much more effective speedup than `chunk`, with substantially worse speedup and times. Interesting, at high numbers of terms, the speedup for `roundRobin` remains the same, while for chunked it was noticeably worse for `chunk`.

Also interesting to note that for `roundRobin`, more speedup was accomplished as terms increased, while the opposite was true for `chunk`

2)
Please find attached our matrix evaluation code in `matrix_partition_c`.