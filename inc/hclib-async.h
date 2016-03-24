/* Copyright (c) 2016, Rice University

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
 * hclib-async.h
 *  
 *      Authors: Vivek Kumar (vivekk@rice.edu), Max Grossman (jmg3@rice.edu)
 *      Acknowledgments: https://wiki.rice.edu/confluence/display/HABANERO/People
 */
#include <functional>

#include "hclib-async-struct.h"
#include "hcupc-support.h"
#include "hclib_promise.h"

#ifndef HCLIB_ASYNC_H_
#define HCLIB_ASYNC_H_

namespace hclib {

/*
 * The C API to the HC runtime defines a task at its simplest as a function
 * pointer paired with a void* pointing to some user data. This file adds a C++
 * wrapper over that API by passing the C API a lambda-caller function and a
 * pointer to the lambda stored on the heap, which are then called.
 *
 * This does add more overheads in the C++ version (i.e. memory allocations).
 * TODO optimize that overhead.
 */

/*
 * At the lowest layer in the call stack before entering user code, this method
 * invokes the user-provided lambda.
 */
template <typename T>
inline void call_lambda(T* lambda) {
	const int wid = current_ws()->id;
	MARK_BUSY(wid);
	(*lambda)();
	HC_FREE((void*) lambda);
	MARK_OVH(wid);
}

/*
 * Store a reference to the type-specific function for calling the user lambda,
 * as well as a pointer to the lambda's location on the heap (through which we
 * can invoke it). async_arguments is stored as the args field in the task_t
 * object for a task, and passed to lambda_wrapper.
 */
template <typename Function, typename T1>
struct async_arguments {
    Function lambda_caller;
    T1 lambda_on_heap;

    async_arguments(Function k, T1 a) :
        lambda_caller(k), lambda_on_heap(a) { }
};

/*
 * The method called directly from the HC runtime, passed a pointer to an
 * async_arguments object. It then uses these async_arguments to call
 * call_lambda, passing the user-provided lambda.
 */
template<typename Function, typename T1>
void lambda_wrapper(void *args) {
    async_arguments<Function, T1> *a =
        (async_arguments<Function, T1> *)args;

    (*a->lambda_caller)(a->lambda_on_heap);
}

/*
 * Initialize a task_t for the C++ APIs, using a user-provided lambda.
 */
template<typename Function, typename T1>
inline void initialize_task(hclib_task_t *t, Function lambda_caller,
        T1 *lambda_on_heap) {
    async_arguments<Function, T1 *> *args =
        new async_arguments<Function, T1*>(lambda_caller, lambda_on_heap);
    t->_fp = lambda_wrapper<Function, T1 *>;
    t->args = args;
    t->is_async_any_type = 0;
    t->future_list = NULL;
    t->place = NULL;
}

template <typename T>
inline hclib_task_t* _allocate_async_hclib(T lambda, bool await) {
	const size_t hclib_task_size = await ? sizeof(hclib_dependent_task_t) : sizeof(hclib_task_t);
    // create off-stack storage for this task
	hclib_task_t* task = (hclib_task_t*)HC_MALLOC(hclib_task_size);
	const size_t lambda_size = sizeof(T);
    /*
     * create off-stack storage for the lambda object (including its captured
     * variables), which will be pointed to from the task_t.
     */
	T* lambda_on_heap = (T*)HC_MALLOC(lambda_size);
	memcpy(lambda_on_heap, &lambda, lambda_size);

    hclib_task_t t;
    initialize_task(&t, call_lambda<T>, lambda_on_heap);
	memcpy(task, &t, sizeof(hclib_task_t));
	return task;
}

#if defined(HUPCPP) && defined(DIST_WS)	// i.e. if we are supporting distributed work-stealing in HabaneroUPC++
#define _allocate_async _allocate_async_hcupc
#else
#define _allocate_async _allocate_async_hclib
#endif

/*
 * Yes, the name "async_at_hpt" sounds weird
 * but using this name to differentiate with the inter-node
 * "asyncAt" in HabaneroUPC++. Feel free to give a better
 * name to async_at_hpt.
 */
template <typename T>
inline void async_at_hpt(place_t* pl, T lambda) {
    MARK_OVH(current_ws()->id);
    hclib_task_t* task = _allocate_async<T>(lambda, false);
    spawn_at_hpt(pl, task);
}

template <typename T>
inline void async(T lambda) {
	MARK_OVH(current_ws()->id);
	hclib_task_t* task = _allocate_async<T>(lambda, false);
	spawn(task);
}

inline int _count_futures() {
    return 0;
}
template <typename... future_list_t>
inline int _count_futures(hclib::future_t *future,
        future_list_t... futures) {
    return 1 + _count_futures(futures...);
}
template <typename... future_list_t>
inline int count_futures(future_list_t... futures) {
    return _count_futures(futures...);
}

inline void _construct_future_list(int index, hclib_future_t **future_list,
        hclib::future_t *future) {
    future_list[index] = future->internal;
}
template <typename... future_list_t>
inline void _construct_future_list(int index, hclib_future_t **future_list,
        hclib::future_t *future, future_list_t... remaining) {
    future_list[index] = future->internal;
    _construct_future_list(index + 1, future_list, remaining...);
}

template <typename... future_list_t>
inline hclib_future_t **construct_future_list(future_list_t... futures) {
    const int nfutures = count_futures(futures...);
    hclib_future_t **future_list = (hclib_future_t **)malloc(
            (nfutures + 1) * sizeof(hclib_future_t *));
    HASSERT(future_list);
    _construct_future_list(0, future_list, futures...);
    future_list[nfutures] = NULL;
    return future_list;
}

template <typename T>
inline void async_await(T lambda, hclib_future_t **future_list) {
	MARK_OVH(current_ws()->id);
	hclib_task_t* task = _allocate_async<T>(lambda, true);
	spawn_await(task, future_list);
}

template <typename T, typename... future_list_t>
inline void async_await(T lambda, future_list_t... futures) {
    hclib_future_t **future_list = construct_future_list(futures...);
    async_await(lambda, future_list);
}

template <typename T>
inline void async_await_at(T lambda, place_t *pl,
        hclib_future_t **future_list) {
	MARK_OVH(current_ws()->id);
	hclib_task_t* task = _allocate_async<T>(lambda, true);
	spawn_await_at(task, future_list, pl);
}

template <typename T, typename... future_list_t>
inline void async_await_at(T lambda, place_t *pl, future_list_t... futures) {
    hclib_future_t **future_list = construct_future_list(futures...);
    async_await_at(lambda, pl, future_list);
}

template <typename T>
inline void async_comm(T lambda) {
	hclib_task_t* task = _allocate_async<T>(lambda, false);
	spawn_comm_task(task);
}

template <typename T>
hclib::future_t *async_future(T lambda) {
    hclib::promise_t *event = new hclib::promise_t();
    hclib_promise_t *internal_event = &event->internal;
    /*
     * TODO creating this closure may be inefficient. While the capture list is
     * precise, if the user-provided lambda is large then copying it by value
     * will also take extra time.
     */
    auto wrapper = [internal_event, lambda]() {
        lambda();
        hclib_promise_put(internal_event, NULL);
    };
    hclib_task_t* task = _allocate_async(wrapper, false);
    spawn(task);
    return event->get_future();
}

template <typename T, typename... future_list_t>
hclib::future_t *async_future_await(T lambda, future_list_t... futures) {
    hclib::promise_t *event = new hclib::promise_t();
    hclib_promise_t *internal_event = &event->internal;
    /*
     * TODO creating this closure may be inefficient. While the capture list is
     * precise, if the user-provided lambda is large then copying it by value
     * will also take extra time.
     */
    auto wrapper = [internal_event, lambda]() {
        lambda();
        hclib_promise_put(internal_event, NULL);
    };

    hclib_future_t **future_list = construct_future_list(futures...);

    hclib_task_t* task = _allocate_async(wrapper, true);
    spawn_await(task, future_list);
    return event->get_future();
}

inline void finish(std::function<void()> lambda) {
    hclib_start_finish();
    lambda();
    hclib_end_finish();
}

inline hclib::future_t *nonblocking_finish(std::function<void()> lambda) {
    hclib_start_finish();
    lambda();
    hclib::promise_t *event = new hclib::promise_t();
    hclib_end_finish_nonblocking_helper(&event->internal);
    return event->get_future();
}

}

#endif /* HCLIB_ASYNC_H_ */
