.global rdtsc

.type rdtsc, @function
rdtsc:
    rdtsc
    ret
.size rdtsc, .-rdtsc
