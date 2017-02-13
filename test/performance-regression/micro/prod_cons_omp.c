#include "hclib.h"

#include <omp.h>
#include <stdio.h>
#include "prod_cons.h"

int main(int argc, char **argv) {
    int i;

    int nthreads;
#pragma omp parallel
#pragma omp master
    {
        nthreads = omp_get_num_threads();
    }
    printf("Using %d OpenMP threads\n", nthreads);


#pragma omp parallel
#pragma omp master
    {
        volatile int *signals = (volatile int *)calloc(PROD_CONS_MSGS,
                sizeof(int));
        assert(signals);

        const unsigned long long start_time = hclib_current_time_ns();
#pragma omp task
        {
            // Producer task
            int i;
            for (i = 0; i < PROD_CONS_MSGS; i++) {
                signals[i] = 1;
            }
        }

        int incr = 0;
        int i;
        for (i = 0; i < PROD_CONS_MSGS; i++) {
            while (signals[i] == 0) ;

#pragma omp task firstprivate(incr)
            {
                // Consumer
                incr = incr + 1;
            }
        }


#pragma omp taskwait
        const unsigned long long end_time = hclib_current_time_ns();
        printf("OpenMP producer-consumer at rate of %f tasks per us\n",
                (double)PROD_CONS_MSGS / ((double)(end_time - start_time) /
                    1000.0));
    }
}