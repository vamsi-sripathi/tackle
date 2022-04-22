#include "tackle.h"

static int is_thread_floating(int ncores, int *p_ithr_affinity)
{
  for (int i = 0, count = 0; i < ncores; i++) {
    if (p_ithr_affinity[i] != -1) {
      count++;
    }
    if (count > 1) {
      return 1;
    }
  }
  return 0;
}

static int show_multi_column_table(machine_info_t *p_mach_info,
                                   tackle_opts_t *p_tackle_opts,
                                   worker_opts_t *p_workers,
                                   int *p_global_affinity)
{
  int num_workers = p_tackle_opts->num_workers;
  int ncores = p_mach_info->total_log_cores;
  int *p_ithr_affinity;

  if (num_workers > 10) {
    return 1;
  }

  // Multi-column table is only shown when there no floating threads
  // and when all workers are homogenous

  // Check for floating workers
  for (int w = 0, worker_offset = 0; w < num_workers; w++) {
    worker_offset += ((w) ? p_workers[w - 1].num_threads * ncores : 0);
    for (int t = 0; t < p_workers[w].num_threads; t++) {
      p_ithr_affinity = p_global_affinity + worker_offset + t * ncores;
      if (is_thread_floating(ncores, p_ithr_affinity)) {
        return 0;
      }
    }
  }
  // Check for non-homogenous workers
  for (int w = 1, nthrs = p_workers[0].num_threads; w < num_workers; w++) {
    if (p_workers[w].num_threads != nthrs) {
      return 0;
    }
  }
  return 1;
}

static void print_multi_column_table(machine_info_t *p_mach_info,
                                     tackle_opts_t *p_tackle_opts,
                                     worker_opts_t *p_workers,
                                     int *p_global_affinity)
{
  int num_workers = p_tackle_opts->num_workers;
  int ncores = p_mach_info->total_log_cores;
  int worker_nthrs;
  int *p_ithr_affinity;

  printf("\t");
  for (int w = 0; w < num_workers; w++) {
    printf("WID[%d]\t\t", w);
  }
  printf("\n");

  worker_nthrs = p_workers[0].num_threads;
  for (int t = 0; t < worker_nthrs; t++) {
    for (int w = 0, worker_offset = 0; w < num_workers; w++) {
      worker_offset += ((w) ? worker_nthrs * ncores : 0);
      p_ithr_affinity = p_global_affinity + worker_offset + t * ncores;
      if (!w) {
        printf("TID[%d]\t", t);
      }
      for (int c = 0; c < ncores; c++) {
        if (p_ithr_affinity[c] != -1) {
          printf("%d\t\t", p_ithr_affinity[c]);
        }
      }
    }
    printf("\n");
  }
}

void show_affinity(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts,
                   worker_opts_t *p_workers, int *p_global_affinity)
{
  int num_workers = p_tackle_opts->num_workers;
  int ncores = p_mach_info->total_log_cores;
  int worker_nthrs;
  int *p_ithr_affinity;

  printf("\nThread --> Core Affinity Map:\n");

  if (show_multi_column_table(p_mach_info, p_tackle_opts, p_workers,
                              p_global_affinity)) {
    print_multi_column_table(p_mach_info, p_tackle_opts, p_workers,
                             p_global_affinity);
    return;
  }

#if 1
  for (int w = 0, worker_offset = 0; w < num_workers; w++) {
    worker_nthrs = p_workers[w].num_threads;
    worker_offset += ((w) ? p_workers[w - 1].num_threads * ncores : 0);
    for (int t = 0; t < worker_nthrs; t++) {
      p_ithr_affinity = p_global_affinity + worker_offset + t * ncores;
      printf("WID[%d]TID[%d] :", w, t);
      for (int c = 0; c < ncores; c++) {
        if (p_ithr_affinity[c] != -1) {
          printf(" %d", p_ithr_affinity[c]);
        }
      }
      printf("\n");
    }
  }
#else
  for (int w = 0, worker_offset = 0; w < num_workers; w++) {
    worker_nthrs = p_workers[w].num_threads;
    worker_offset += ((w) ? p_workers[w - 1].num_threads * ncores : 0);
    printf("Worker-%d has %d threads existing on following cpus:", w,
           worker_nthrs);
    for (int t = 0; t < worker_nthrs; t++) {
      p_ithr_affinity = p_global_affinity + worker_offset + t * ncores;
      for (int c = 0; c < ncores; c++) {
        if (p_ithr_affinity[c] != -1) {
          printf(" %d", p_ithr_affinity[c]);
        }
      }
    }
    printf("\n");
  }
#endif
}
