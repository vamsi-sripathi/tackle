#include "tackle.h"

int affinity_checker(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts,
                     worker_opts_t *p_workers)
{
  int status, num_workers, total_nthrs, size, empty_workers, inter_op;
  int *p_global_affinity = NULL;
  pid_t pid;

  num_workers = p_tackle_opts->num_workers;
  inter_op = p_tackle_opts->inter_op;

  total_nthrs = 0;
  for (int i = 0; i < num_workers; i++) {
    // For checker functionality, we need to scale num_threads for each worker
    // by inter_op in full tackle mode. We will restore them back to original
    // at the end of this routine after checker finishes
    if (p_tackle_opts->mode == TACKLE) {
      p_workers[i].num_threads *= inter_op;
    }
    total_nthrs += p_workers[i].num_threads;
  }

  size = total_nthrs * p_mach_info->total_log_cores * sizeof(int);
  p_global_affinity = (int *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                  MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  if (p_global_affinity == MAP_FAILED) {
    printf("ERROR: Affinity Checker mmap failed\n");
    return 1;
  }

  empty_workers = 0;
  for (int i = 0; i < num_workers; i++) {
    if (p_workers[i].num_threads) {
      pid = fork();
      if (pid < 0) {
        printf("ERROR: Affinity Checker fork failed\n");
        fflush(0);
        return 1;
      } else if (pid == 0) {
        probe_affinity(i, p_mach_info, p_workers, p_global_affinity);
        exit(0);
      }
    } else {
      empty_workers++;
    }
  }

  for (int i = 0; i < (num_workers - empty_workers); i++) {
    pid = wait(&status);
    if (pid == -1) {
      printf("ERROR: Wait() for %d worker failed\n", i);
      fflush(0);
      return 1;
    }
  }

  show_affinity(p_mach_info, p_tackle_opts, p_workers, p_global_affinity);
  dump_affinity_map(p_mach_info, p_tackle_opts, p_workers, p_global_affinity);
  inspect_affinity(p_mach_info, p_tackle_opts, p_workers, p_global_affinity);

  munmap(p_global_affinity, size);

  if (p_tackle_opts->mode == TACKLE) {
    // Restore original num_threads for all workers
    for (int i = 0; i < num_workers; i++) {
      p_workers[i].num_threads /= inter_op;
    }
  }

  return 0;
}
