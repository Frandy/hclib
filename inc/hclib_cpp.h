#ifndef HCLIB_CPP_H_
#define HCLIB_CPP_H_

#include "hclib_common.h"
#include "hclib-rt.h"
#include "hclib-async.h"
#include "hclib-forasync.h"
#include "hclib_promise.h"
#include "hclib.h"
#include "hclib-locality-graph.h"

namespace hclib {

typedef hclib_triggered_task_t triggered_task_t;
typedef hclib_locale_t locale_t;

template <typename T>
void launch(const char **deps, int ndeps, T lambda) {
    hclib_task_t *user_task = _allocate_async(lambda, false);
    hclib_launch((generic_frame_ptr)spawn, user_task, deps, ndeps);
}

promise_t **promise_create_n(size_t nb_promises, int null_terminated);

extern hclib_worker_state *current_ws();
int get_current_worker();
int num_workers();

int get_num_locales();
hclib_locale_t *get_closest_locale();
hclib_locale_t **get_thread_private_locales();
hclib_locale_t *get_all_locales();
hclib_locale_t **get_all_locales_of_type(int type, int *out_count);

hclib::future_t *allocate_at(size_t nbytes, hclib::locale_t *locale);
hclib::future_t *reallocate_at(void *ptr, size_t nbytes,
        hclib::locale_t *locale);
void free_at(void *ptr, hclib::locale_t *locale);
hclib::future_t *memset_at(void *ptr, int pattern, size_t nbytes,
        hclib::locale_t *locale);

inline hclib::future_t *async_copy_await(hclib::locale_t *dst_locale, void *dst,
        hclib::locale_t *src_locale, void *src, size_t nbytes, hclib::future_t *future) {
    return new hclib::future_t(hclib_async_copy(dst_locale, dst, src_locale,
                src, nbytes, future ? &future->internal : NULL, future ? 1 : 0));
}

inline hclib::future_t *async_copy_await_all(hclib::locale_t *dst_locale,
        void *dst, hclib::locale_t *src_locale, void *src, size_t nbytes,
        hclib::future_t **futures, const int nfutures) {
    hclib_future_t *futures_arr[nfutures];
    for (int i = 0; i < nfutures; i++) {
        futures_arr[i] = futures[i]->internal;
    }

    return new hclib::future_t(hclib_async_copy(dst_locale, dst, src_locale,
                src, nbytes, futures_arr, nfutures));
}


inline hclib::future_t *async_copy(hclib::locale_t *dst_locale, void *dst,
        hclib::locale_t *src_locale, void *src, size_t nbytes) {
    return new hclib::future_t(hclib_async_copy(dst_locale, dst, src_locale,
                src, nbytes, NULL, 0));
}


}

#endif
