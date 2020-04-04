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

#define VMOBJ_NAME "/vmops_bench_shared_independent"

struct bench_run_arg
{
    plat_memobj_t memobj;
    plat_thread_t thread;
    struct vmops_bench_cfg *cfg;
    uint32_t tid;
    plat_barrier_t barrier;
    size_t count;
    double duration;
};

static void *bench_run_fn(void *st)
{
    plat_error_t err;

    struct bench_run_arg *args = st;

    plat_time_t t_delta = plat_convert_time(args->cfg->time_ms);

    size_t counter = 0 ;

    LOG_INFO("thread %d ready.\n", args->tid);
    plat_thread_barrier(args->barrier);


    size_t memsize = args->cfg->memsize;
    plat_time_t t_current = plat_get_time();
    plat_time_t t_end = t_current + t_delta;
    plat_time_t t_start = t_current;

    if (args->cfg->nounmap) {
        while(t_current < t_end) {
            void *addr;
            err = plat_vm_map(&addr, args->cfg->memsize, args->memobj, 0);
            if (err != PLAT_ERR_OK) {
                printf("failed to map! %p\n", addr);
            }
            counter++;
        }
    } else {
        while (t_current < t_end) {
            void *addr;
            err = plat_vm_map(&addr, memsize, args->memobj, 0);
            if (err != PLAT_ERR_OK) {
                LOG_ERR("thread %d. failed to map memory!\n", args->tid);
            }

            err = plat_vm_unmap(addr, memsize);
            if (err != PLAT_ERR_OK) {
                LOG_ERR("thread %d. failed to unmap memory!\n", args->tid);
            }

            t_current = plat_get_time();
            counter++;
        }
    }
    t_end = plat_get_time();

    plat_thread_barrier(args->barrier);

    if (args->cfg->nounmap) {
        printf("thread %d ended. %zu maps\n", args->tid, counter);
    } else {
        printf("thread %d ended. %zu map + unmaps\n", args->tid, counter);
    }

    args->count = counter;
    args->duration = plat_time_to_ms(t_end - t_start);

    LOG_INFO("thread %d done. ops = %zu, time=%.3f\n", args->tid, counter, args->duration);

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
void vmops_bench_run_shared_independent(struct vmops_bench_cfg *cfg)
{
    plat_error_t err;

    LOG_INFO("Preparing 'MAP/UNMAP Shared' benchmark.\n");

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

    LOG_INFO("creating 1 memory objects of size %zu\n", cfg->memsize);
    LOG_INFO("total memory usage = %zu kB\n", (cfg->memsize) >> 10);


    plat_memobj_t mobj;
    err = plat_vm_create(VMOBJ_NAME, &mobj, cfg->memsize);
    if (err != PLAT_ERR_OK) {
        printf(
            "could not create the memobj\n"
        );
        exit(EXIT_FAILURE);
    }

    LOG_INFO("creating %d threads\n", cfg->corelist_size);
    for (uint32_t i = 0; i < cfg->corelist_size; i++) {
        LOG_INFO("thread %d on core %d\n", i, cfg->coreslist[i]);
        args[i].tid = i;
        args[i].cfg = cfg;
        args[i].barrier = barrier;
        args[i].memobj = mobj;
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

    LOG_INFO("cleaning up memory objects\n");
    plat_vm_destroy(mobj);

    plat_thread_barrier_destroy(barrier);



    LOG_INFO("Benchmark done. total ops = %zu\n", total_ops);

    LOG_CSV_HEADER();
    for (uint32_t i = 0; i < cfg->corelist_size; i++) {
        LOG_CSV(cfg->benchmark, i, cfg->coreslist[i],
            cfg->corelist_size, cfg->memsize, args[i].duration, args[i].count);
    }
    LOG_CSV_FOOTER();
    LOG_RESULT(cfg->benchmark, cfg->memsize, cfg->time_ms, cfg->corelist_size, total_ops);

    free(args);
}