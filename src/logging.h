/*
 * Virtual Memory Operations Benchmark
 *
 * Copyright 2020 Reto Achermann
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef __VMOPS_LOGGING_H_
#define __VMOPS_LOGGING_H_ 1

#include <stdio.h>
#include <inttypes.h>


// General Formatting
#define FORMAT_RESET "0"
#define FORMAT_BRIGHT "1"
#define FORMAT_DIM "2"
#define FORMAT_ITALIC "3"
#define FORMAT_UNDERSCORE "4"
#define FORMAT_REVERSE "5"
#define FORMAT_HIDDEN "6"

// Foreground Colors
#define COLOR_BLACK "30"
#define COLOR_RED "31"
#define COLOR_GREEN "32"
#define COLOR_YELLOW "33"
#define COLOR_BLUE "34"
#define COLOR_MAGENTA "35"
#define COLOR_CYAN "36"
#define COLOR_WHITE "37"


#define SHELL_COLOR_ESCAPE_SEQ(X) "\x1b[" X "m"

#define SHELL_FORMAT_RESET ANSI_COLOR_ESCAPE_SEQ(GEN_FORMAT_RESET)

#define BUILD_FORMAT(_STYLE, _COLOR) "\x1b[" _STYLE ";" _COLOR "m"

#define COLOR_RESET "\x1b[" FORMAT_RESET "m"
#define COLOR_INFO BUILD_FORMAT(FORMAT_BRIGHT, COLOR_GREEN)
#define COLOR_WARN BUILD_FORMAT(FORMAT_BRIGHT, COLOR_YELLOW)
#define COLOR_ERR BUILD_FORMAT(FORMAT_BRIGHT, COLOR_RED)
#define COLOR_RESULT                                                                              \
    BUILD_FORMAT(FORMAT_BRIGHT, COLOR_CYAN) BUILD_FORMAT(FORMAT_UNDERSCORE, COLOR_CYAN)

#define VMOPS_PRINT_PREFIX "+VMOPS "

#define LOG_INFO(...)                                                                             \
    fprintf(stderr, VMOPS_PRINT_PREFIX COLOR_INFO "INFO " COLOR_RESET __VA_ARGS__)

#define LOG_WARN(...)                                                                             \
    fprintf(stderr, VMOPS_PRINT_PREFIX COLOR_WARN "WARN " COLOR_RESET __VA_ARGS__)

#define xstr(a) str(a)
#define str(a) #a
#define LOG_ERR(...)                                                                              \
    fprintf(stderr, VMOPS_PRINT_PREFIX COLOR_ERR "ERROR " COLOR_RESET __FILE__                    \
                                                 ":" xstr(__LINE__) " " __VA_ARGS__)


#define LOG_PRINT(...) fprintf(stderr, VMOPS_PRINT_PREFIX __VA_ARGS__)
#define LOG_PRINT_CONT(...) fprintf(stderr, __VA_ARGS__)
#define LOG_PRINT_END(...) fprintf(stderr, __VA_ARGS__)


#define RESULT_FMT_STRING                                                                         \
    "benchmark=%s, memsize=%zu, time=%.2f, ncores=%d, ops=%zu, thpt=%.2f, lat=%.4f"

#define LOG_RESULT(_b, _m, _t, _n, _o, _thpt, _lat)                                                \
    fprintf(stderr,                                                                                \
            VMOPS_PRINT_PREFIX COLOR_RESULT "RESULT [[ " RESULT_FMT_STRING " ]]" COLOR_RESET "\n", \
            _b, _m, _t, _n, _o, _thpt, _lat)


/*
 * ================================================================================================
 * Printing of Benchmark Throughput Results in CSV
 * ================================================================================================
 */


///< file set to where the throughput csv is printed, defined in main.c
extern FILE *thptout;

#define LOG_CSV_HEADER()                                                                          \
    fprintf(stderr, "===================== BEGIN CSV =====================\n");                   \
    fprintf(thptout, "thread_id,benchmark,core,ncores,memsize,numainterleave,mappings_size,page_" \
                    "size,memobj,isolation,duration,operations\n");

#define LOG_CSV_FOOTER()                                                                          \
    fprintf(stderr, "====================== END CSV ======================\n");

// If you modify the CSV format, also change the header-line in scripts/run.sh accordingly:
#define LOG_CSV(_cfg, _t, _d, _tpt)                                                               \
    fprintf(thptout, "%d,%s,%d,%d,%zu,%s,%s,%s,%s,%s,%.3f,%zu\n", _t, (_cfg)->benchmark,          \
            (_cfg)->coreslist[_t], (_cfg)->corelist_size, (_cfg)->memsize,                        \
            ((_cfg)->numainterleave ? "numainterleave" : "numafill"),                             \
            ((_cfg)->map4k ? "smallmappings" : "onelargemap"),                                    \
            ((_cfg)->maphuge ? "hugepages" : "basepages"),                                        \
            ((_cfg)->shared ? "shared-memobj" : "independent-memobj"),                            \
            ((_cfg)->isolated ? "isolated" : "default"), _d, _tpt);


/*
 * ================================================================================================
 * Printing of Benchmark Latency Results in CSV
 * ================================================================================================
 */


///< file set to where the latency csv is printed, defined in main.c
extern FILE *latout;

// prints time elapsed, thread id, number of operations so far on this thread, time of the operation
#define LOG_STATS_HEADER()                                                                        \
    fprintf(stderr, "====================== BEGIN STATS ======================\n");               \
    fprintf(latout, "benchmark,core,ncores,memsize,numainterleave,mappings_size,page_"           \
                    "size,memobj,isolation,threadid,elapsed,couter,latency\n");

#define LOG_STATS_FOOTER()                                                                        \
    fprintf(stderr, "====================== END STATS ======================\n");


#define LOG_STATS(_cfg, n, stat)                                                                  \
    do {                                                                                          \
        if ((stat).t_elapsed != 0 && (stat).val != 0) {                                           \
            fprintf(latout, "%s,%d,%d,%zu,%s,%s,%s,%s,%s,%d,%f,%" PRIu64 ",%f\n",                 \
                    (_cfg)->benchmark, (_cfg)->coreslist[(stat).tid], (_cfg)->corelist_size,      \
                    (_cfg)->memsize, ((_cfg)->numainterleave ? "numainterleave" : "numafill"),    \
                    ((_cfg)->map4k ? "smallmappings" : "onelargemap"),                            \
                    ((_cfg)->maphuge ? "hugepages" : "basepages"),                                \
                    ((_cfg)->shared ? "shared-memobj" : "independent-memobj"),                    \
                    ((_cfg)->isolated ? "isolated" : "default"), (stat).tid,                      \
                    plat_time_to_ms((stat).t_elapsed), (stat).counter,                            \
                    plat_time_to_ms((stat).val));                                                 \
        }                                                                                         \
    } while (0)


#endif /* __VMOPS_LOGGING_H_ */
