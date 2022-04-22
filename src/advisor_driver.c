#include "tackle.h"

int advisor_driver(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts)
{
  worker_opts_t *p_workers = NULL;

  p_workers = (worker_opts_t *)malloc(sizeof(worker_opts_t) *
                                      p_tackle_opts->num_workers);
  if (p_workers == NULL) {
    printf("\nERROR: Affinity Advisor memory allocation failed\n");
    exit(1);
  }

  affinity_advisor(p_mach_info, p_tackle_opts, p_workers);

  free(p_workers);
  return 0;
}
