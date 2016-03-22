/* Copyright (c) 2015, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Rice University
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

/*
 * ocr-runtime.cpp
 *
 *      Author: Vivek Kumar (vivekk@rice.edu)
 *      Acknowledgments: https://wiki.rice.edu/confluence/display/HABANERO/People
 */

#include "hcpp.h"
#include <sys/time.h>

namespace hcpp {
using namespace std;
#define ASYNC_COMM ((int) 0x2)

static double benchmark_start_time_stats = 0;

void execute_task(void* t) {
	task_t* task = (task_t*) t;
	(task->_fp)(task->_args);
	HC_FREE((void*) task);
}

void spawn(task_t * task) {
	::async(&execute_task, (void *) task, NO_DDF, NO_PHASER, NO_PROP);
}

void spawnComm(task_t * task) {
	::async(&execute_task, (void *) task, NO_DDF, NO_PHASER, ASYNC_COMM);
}

void spawn_await(task_t * task, DDF_t ** ddf_list) {
	::async(&execute_task, (void *) task, (struct ddf_st**) ddf_list, NO_PHASER, NO_PROP);
}

double mysecond() {
        struct timeval tv;
        gettimeofday(&tv, 0);
        return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}

void display_runtime() {
        cout << "---------HCPP_OCR_RUNTIME_INFO-----------" << endl;
        printf(">>> HCPP_WORKERS\t\t= %s\n",getenv("OCR_CONFIG"));
        printf(">>> HCPP_STATS\t\t= %s\n",getenv("HCPP_STATS"));
        cout << "----------------------------------------" << endl;
}

void showStatsHeader() {
        cout << endl;
        cout << "-----" << endl;
        cout << "mkdir timedrun fake" << endl;
        cout << endl;
        cout << "-----" << endl;
        benchmark_start_time_stats = mysecond();
}

void runtime_statistics(double duration) {
        printf("============================ MMTk Statistics Totals ============================\n");
        printf("time.mu\ttotalPushOutDeq\ttotalPushInDeq\ttotalStealsInDeq\n");
        printf("%.3f\t0\t0\t0\n",duration);
        printf("Total time: %.3f ms\n",duration);
        printf("------------------------------ End MMTk Statistics -----------------------------\n");
        printf("===== TEST PASSED in %.3f msec =====\n",duration);
}

void showStatsFooter() {
        double end = mysecond();
        HASSERT(benchmark_start_time_stats != 0);
        double dur = (end-benchmark_start_time_stats)*1000;
        runtime_statistics(dur);
}

void init(int * argc, char ** argv) {
        if(getenv("HCPP_STATS")) {
                showStatsHeader();
                display_runtime();
        }
	hclib_init(argc, argv);
}

void finalize() {
        if(getenv("HCPP_STATS")) {
                showStatsFooter();
        }
	hclib_finalize();
}

void start_finish() {
	::start_finish();
}

void end_finish() {
	::end_finish();
}

void finish(std::function<void()> lambda) {
	start_finish();
	lambda();
	end_finish();
}

int get_hc_wid() {
	return get_worker_id(); 
	return 0;
}

int numWorkers() {
	return get_nb_workers();
	return 0;
}

DDF_t * ddf_create() {
	return ::ddf_create();
}

/**
 * @brief Allocate and initialize an array of DDFs.
 * @param[in] nb_ddfs                           Size of the DDF array
 * @param[in] null_terminated           If true, create nb_ddfs-1 and set the last element to NULL.
 * @return A contiguous array of DDFs
 */
DDF_t ** ddf_create_n(size_t nb_ddfs, int null_terminated) {
	return ::ddf_create_n(nb_ddfs, null_terminated);
}

/**
 * @brief Destruct a DDF.
 * @param[in] ddf                               The DDF to destruct
 */
void ddf_free(DDF_t * ddf) {
	::ddf_free(ddf);
}

/**
 * @brief Get the value of a DDF.
 * @param[in] ddf                               The DDF to get a value from
 */
void * ddf_get(DDF_t * ddf) {
	return ::ddf_get(ddf);
}

/**
 * @brief Put a value in a DDF.
 * @param[in] ddf                               The DDF to get a value from
 * @param[in] datum                     The datum to be put in the DDF
 */
void ddf_put(DDF_t * ddf, void * datum) {
	::ddf_put(ddf, datum);
}

}
