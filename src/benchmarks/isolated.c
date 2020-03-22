/*
 * Virtual Memory Operations Benchmark
 *
 * Copyright 2020 Reto Achermann
 * SPDX-License-Identifier: GPL-3.0
 */


#include <stdint.h>
#include <stdlib.h>

#include "benchmarks.h"
#include "utils.h"

/**
 * @brief runs the vmops benchmark in the isolated configuration
 *
 * @param cfg   the benchmark configuration
 *
 *  - each thread has an independent memory object to be mapped
 *  - each thread maps it in a distinct slot in the root page table
 */
void vmops_bench_run_isolated(struct vmops_bench_cfg *cfg)
{
    (void)(cfg);
}