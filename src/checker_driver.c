#include "tackle.h"

int checker_driver(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts)
{
  worker_opts_t *p_workers = NULL;

  int num_workers = p_tackle_opts->num_workers;

  p_workers = (worker_opts_t *)malloc(sizeof(worker_opts_t) * num_workers);
  if (p_workers == NULL) {
    printf("ERROR: Affinity Checker memory allocation failed\n");
    return 1;
  }

  for (int i = 0; i < num_workers; i++) {
    if (!p_tackle_opts->omp_num_threads[i]) {
      // If user did not set omp_num_threads, then default to system total cores
      p_workers[i].num_threads = p_mach_info->total_log_cores;
    } else {
      p_workers[i].num_threads = p_tackle_opts->omp_num_threads[i];
    }

    if (p_tackle_opts->omp_affinity[i]) {
      strcpy(p_workers[i].affinity_str, p_tackle_opts->omp_affinity[i]);
    } else {
      *p_workers[i].affinity_str = '\0';
    }
  }

  affinity_checker(p_mach_info, p_tackle_opts, p_workers);

  return 0;
}
