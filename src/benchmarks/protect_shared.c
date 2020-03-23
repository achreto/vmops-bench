/*
 * Virtual Memory Operations Benchmark
 *
 * Copyright 2020 Reto Achermann
 * SPDX-License-Identifier: GPL-3.0
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "benchmarks.h"
#include "utils.h"

#define VMOBJ_NAME "/vmops_bench_protect_shared"

struct bench_run_arg
{
    void *addr;
    plat_thread_t thread;
    size_t memsize;
    uint32_t time_ms;
    uint32_t tid;
    plat_barrier_t barrier;
    size_t count;
};

static void *bench_run_fn(void *st)
{
    plat_error_t err;

    struct bench_run_arg *args = st;

    plat_time_t t_delta = plat_convert_time(args->time_ms);

    size_t counter = 0 ;

    void *addr = args->addr;
    LOG_INFO("thread %d ready.\n", args->tid);
    plat_thread_barrier(args->barrier);


    plat_time_t t_current = plat_get_time();
    plat_time_t t_end = t_current + t_delta;

    while(t_current < t_end) {
        err = plat_vm_protect(addr, args->memsize, PLAT_PERM_READ_ONLY);
        if (err != PLAT_ERR_OK) {
            LOG_ERR("thread %d. failed to protect memory!\n", args->tid);
            exit(EXIT_FAILURE);
        }
        err = plat_vm_protect(addr, args->memsize, PLAT_PERM_READ_WRITE);
        if (err != PLAT_ERR_OK) {
            LOG_ERR("thread %d. failed to unprotect memory!\n", args->tid);
            exit(EXIT_FAILURE);
        }
        t_current = plat_get_time();
        counter++;
    }

    plat_thread_barrier(args->barrier);

    LOG_INFO("thread %d done. ops = %zu\n", args->tid, counter);

    args->count = counter;

    return NULL;
}


/**
 * @brief runs the vmops benchmark in the concurrent protect configuration
 *
 * @param cfg   the benchmark configuration
 *
 *  - there is a single shared memory region
 *  - calling randomly protect() on pages.
 */
void vmops_bench_run_protect_shared(struct vmops_bench_cfg *cfg)
{
    plat_error_t err;

    LOG_INFO("Preparing 'Protect Shared' benchmark.\n");

    plat_barrier_t barrier;
    err = plat_thread_barrier_init(&barrier, cfg->corelist_size);
    if (err != PLAT_ERR_OK) {
        exit(EXIT_FAILURE);
    }

    /* create the threads */
    struct bench_run_arg *args = malloc(cfg->corelist_size * sizeof(struct bench_run_arg));
    if (args == NULL) {
        exit(EXIT_FAILURE);
    }

    size_t totalmem = cfg->corelist_size * cfg->memsize;
    LOG_INFO("creating %d memory objects of size %zu\n", cfg->corelist_size, cfg->memsize);
    LOG_INFO("total memory usage = %zu kB\n", totalmem >> 10);

    plat_memobj_t memobj;
    err = plat_vm_create(VMOBJ_NAME, &memobj, totalmem);
    if (err != PLAT_ERR_OK) {
        LOG_ERR("creation of shared memory object failed!\n");
        exit(EXIT_FAILURE);
    }

    void *addr;
    err = plat_vm_map(&addr, totalmem, memobj, 0);
    if  (err != PLAT_ERR_OK) {
        LOG_ERR("creation of shared memory object failed!\n");
        plat_vm_destroy(memobj);
        exit(EXIT_FAILURE);
    }

    LOG_INFO("creating %d threads\n", cfg->corelist_size);
    for (uint32_t i = 0; i < cfg->corelist_size; i++) {
        LOG_INFO("thread %d on core %d\n", i, cfg->coreslist[i]);
        args[i].tid = i;
        args[i].addr = (void *)((uintptr_t)addr + i * cfg->memsize);
        args[i].memsize = cfg->memsize;
        args[i].time_ms = cfg->time_ms;
        args[i].barrier = barrier;
        args[i].thread = plat_thread_start(bench_run_fn, &args[i], cfg->coreslist[i]);
        if (args[i].thread == NULL) {
            LOG_ERR("failed to start threads! [%d / %d]\n", i, cfg->corelist_size);
            for (uint32_t j = 0; j < i; j++) {
                plat_thread_cancel(args[j].thread);
            }
        }
    }

    size_t total_ops = 0;
    for (uint32_t i = 0; i < cfg->corelist_size; i++) {
        if (args[i].thread != NULL) {
            plat_thread_join(args[i].thread);
            total_ops += args[i].count;
        }
    }

    plat_vm_unmap(addr, totalmem);

    LOG_INFO("cleaning up memory objects\n");
    plat_vm_destroy(memobj);

    plat_thread_barrier_destroy(barrier);

    LOG_RESULT(cfg->benchmark, cfg->memsize, cfg->time_ms, cfg->corelist_size, total_ops);
    LOG_INFO("Benchmark done. total ops = %zu\n", total_ops);

    free(args);
}