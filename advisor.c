#include "tackle.h"

static void show_workers_config(tackle_opts_t *p_tackle_opts, worker_opts_t *p_workers)
{
  printf("Advisor Recommendations:\n");
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    printf("WID[%d]:\n", i);
    if (p_workers[i].num_threads) {
      if (p_tackle_opts->inter_op > 1) {
        printf("\tTotal Threads     = %d\n", p_workers[i].num_threads*p_tackle_opts->inter_op);
      }
      printf("\tOMP_NUM_THREADS   = %s\n", p_workers[i].num_threads_str);
      printf("\tOMP Affinity      = %s\n", p_workers[i].affinity_str);
      printf("\tTaskset CPU(s)    = %s\n", p_workers[i].cpus_str);
      printf("\tNUMA membind      = %s\n", p_workers[i].numa_str);
    } else {
      printf("\tOMP Threads       = 0\n");
    }
    fflush(0);
  }
}

static int populate_workers(machine_info_t *p_mach_info,
                            tackle_opts_t *p_tackle_opts,
                            worker_opts_t *p_workers)
{
  int total_cpus;
  int *cpus_list = NULL;

#if defined (TACKLE_USE_LOGICAL_CORES_IN_OMP)
  total_cpus = p_mach_info->total_log_cores;
  cpus_list  = p_mach_info->core_siblings;
#else
  total_cpus = p_mach_info->total_phy_cores;
  cpus_list = (int *)malloc(sizeof(int) * total_cpus);
  if (cpus_list == NULL) {
    printf("ERROR: Memory allocation failed in Affinity Advisor\n");
    return 1;
  }

  for (int i = 0; i < total_cpus; i++) {
    cpus_list[i] =
        p_mach_info->core_siblings[i * p_mach_info->threads_per_core];
  }
#endif

  bool use_log_cores = false;
#if defined (TACKLE_USE_LOGICAL_CORES_IN_TASKSET) && \
    !defined (TACKLE_USE_LOGICAL_CORES_IN_OMP)
  use_log_cores = ((p_mach_info->is_ht) ? true : false);
#endif

  for (int i = 0, count = 0; i < p_tackle_opts->num_workers; i++) {
    int numa_node;
    bool multiple_numa_nodes = false;
    int worker_nthrs = p_workers[i].num_threads * p_tackle_opts->inter_op;
    char log_cpus_str[TACKLE_MAX_BUFFER_LENGTH];
    int sloc = 0;
    for (int t = 0, sloc2 = 0, cpu_id_start, cpu_id_end;
         t < worker_nthrs && count < total_cpus; t++, count++) {
      if (t == 0) {
        numa_node = map_cpu_to_numa_node(cpus_list[count], p_mach_info);
        cpu_id_start = cpus_list[count];
        if (worker_nthrs == 1) {
          sloc += sprintf(p_workers[i].cpus_str, "%d", cpu_id_start);
          if (use_log_cores) {
            sprintf(log_cpus_str, "%d", get_ht_sibling(cpu_id_start, p_mach_info));
          }
        }
#if !defined (TACKLE_USE_SHORT_CPU_LIST)
        if (t < worker_nthrs - 1) {
          sloc += sprintf(p_workers[i].cpus_str + sloc, "%d,", cpu_id_start);
          if (use_log_cores) {
            sloc2 += sprintf(log_cpus_str + sloc2, "%d,", get_ht_sibling(cpu_id_start, p_mach_info));
          }
        }
#endif
        continue;
      }

      if (map_cpu_to_numa_node(cpus_list[count], p_mach_info) != numa_node) {
        multiple_numa_nodes = true;
      }

#if defined (TACKLE_USE_SHORT_CPU_LIST)
      if (cpus_list[count] - cpus_list[count-1] != 1) {
        cpu_id_end = cpus_list[count-1];
        if (cpu_id_start == cpu_id_end) {
          sloc += sprintf(p_workers[i].cpus_str + sloc, "%d", cpu_id_start);
          if (use_log_cores) {
            sloc2 += sprintf(log_cpus_str + sloc2, "%d",
                             get_ht_sibling(cpu_id_start, p_mach_info));
          }
        } else {
          sloc += sprintf(p_workers[i].cpus_str + sloc, "%d-%d",
                           cpu_id_start, cpu_id_end);
          if (use_log_cores) {
            sloc2 += sprintf(log_cpus_str + sloc2, "%d-%d",
                             get_ht_sibling(cpu_id_start, p_mach_info),
                             get_ht_sibling(cpu_id_end, p_mach_info));
          }
        }
        if (t < worker_nthrs) {
          sloc += sprintf(p_workers[i].cpus_str + sloc, ",");
          if (use_log_cores) {
            sloc2 += sprintf(log_cpus_str + sloc2, ",");
          }
        }
        cpu_id_start = cpus_list[count];
      }

      if (t == worker_nthrs - 1) {
        cpu_id_end = cpus_list[count];
        if (cpu_id_start == cpu_id_end) {
          sloc += sprintf(p_workers[i].cpus_str + sloc, "%d", cpu_id_start);
          if (use_log_cores) {
            sloc2 += sprintf(log_cpus_str + sloc2, "%d",
                             get_ht_sibling(cpu_id_start, p_mach_info));
          }
        } else {
          sloc += sprintf(p_workers[i].cpus_str + sloc, "%d-%d",
                           cpu_id_start, cpu_id_end);
          if (use_log_cores) {
            sloc2 += sprintf(log_cpus_str + sloc2, "%d-%d",
                            get_ht_sibling(cpu_id_start, p_mach_info),
                            get_ht_sibling(cpu_id_end, p_mach_info));
          }
        }
      }
#else
      if (t == worker_nthrs - 1) {
        sloc += sprintf(p_workers[i].cpus_str + sloc, "%d", cpus_list[count]);
        if (use_log_cores) {
          sloc2 += sprintf(log_cpus_str + sloc2, "%d",
                           get_ht_sibling(cpus_list[count], p_mach_info));
        }
      } else {
        sloc += sprintf(p_workers[i].cpus_str + sloc, "%d,", cpus_list[count]);
        if (use_log_cores) {
          sloc2 += sprintf(log_cpus_str + sloc2, "%d,",
                           get_ht_sibling(cpus_list[count], p_mach_info));
        }
      }
#endif
    }

    if (worker_nthrs > 0) {
      sprintf(p_workers[i].num_threads_str, "%d", p_workers[i].num_threads);
      sprintf(p_workers[i].affinity_str,
              "granularity=fine,explicit,proclist=[%s]", p_workers[i].cpus_str);
      if (use_log_cores) {
        sprintf(p_workers[i].cpus_str+sloc, ",%s", log_cpus_str);
      }

      if (multiple_numa_nodes) {
        sprintf(p_workers[i].numa_str, "-l");
      } else {
        sprintf(p_workers[i].numa_str, "-m%d", numa_node);
      }
    }

  }

  if (cpus_list != p_mach_info->core_siblings) {
    free(cpus_list);
  }
  return 0;
}

static int initialize_workers(machine_info_t *p_mach_info,
                              tackle_opts_t *p_tackle_opts,
                              worker_opts_t *p_workers)
{
  int nthrs_tail, nthrs_per_worker, num_workers_with_tail, total_nthrs;
  int num_workers = p_tackle_opts->num_workers;
  int inter_op = p_tackle_opts->inter_op;
  bool user_set_nthrs = true;
  bool user_set_aff = true;
  int total_cpus;

#if defined (TACKLE_USE_LOGICAL_CORES_IN_OMP)
  total_cpus = p_mach_info->total_log_cores;
#else
  total_cpus = p_mach_info->total_phy_cores;
#endif

  // Init all workers to empty
  for (int i = 0; i < num_workers; i++) {
    *p_workers[i].num_threads_str = '\0';
    *p_workers[i].affinity_str = '\0';
    *p_workers[i].cpus_str = '\0';
    *p_workers[i].numa_str = '\0';
  }

  for (int i = 0; i < num_workers; i++) {
    if (p_tackle_opts->omp_num_threads[i]) {
      p_workers[i].num_threads = p_tackle_opts->omp_num_threads[i] / inter_op;
      sprintf(p_workers[i].num_threads_str, "%d", p_workers[i].num_threads);
      if (p_tackle_opts->omp_affinity[i]) {
        strcpy(p_workers[i].affinity_str, p_tackle_opts->omp_affinity[i]);
      } else {
        user_set_aff = false;
      }
    } else {
      user_set_nthrs = false;
      user_set_aff = false;
    }
    if (!user_set_nthrs && !user_set_aff) {
      break;
    }
  }

  if (user_set_nthrs && user_set_aff) {
    printf(
        "Advisor Warning: User specified number of workers, number of"
        " threads per worker and corresponding affinity. There is nothing"
        " to advice, skipping advisor!\n");
    fflush(0);
    return 1;
  }

  if (user_set_nthrs) {
    total_nthrs = 0;
    for (int i = 0; i < num_workers; i++) {
      total_nthrs += p_workers[i].num_threads * inter_op;
    }

    if (total_nthrs > total_cpus) {
      printf(
          "Advisor Warning: User specified number of workers and threads"
          " leads to system over-subscription. Reducing number of threads\n");
      fflush(0);
    } else {
      return 0;
    }
  }

  nthrs_per_worker = total_cpus / (num_workers * inter_op);
  for (int i = 0; i < num_workers; i++) {
    p_workers[i].num_threads = nthrs_per_worker;
  }

  nthrs_tail = total_cpus - (nthrs_per_worker * num_workers * inter_op);
  num_workers_with_tail = nthrs_tail / inter_op;
#ifdef TACKLE_HETERO_WORKERS
  for (int i = 0; i < num_workers_with_tail; i++) {
    p_workers[i].num_threads++;
  }
#endif

  if (num_workers > total_cpus) {
    printf(
        "Advisor Warning: The number of available cores (%d) are less than"
        " requested number of total workers (%d). %d workers will have zero"
        " threads\n",
        total_cpus, num_workers, num_workers - total_cpus);
    fflush(0);
    return 0;
  }

  if (num_workers_with_tail) {
#ifdef TACKLE_HETERO_WORKERS
    printf(
        "Advisor Warning: The number of available cores (%d) cannot be"
        " evenly distributed to requested number of workers (%d)\n"
        "                 Distributing %d tail cores across %d workers."
        " This leads to heterogenous workers!\n",
        total_cpus, num_workers, nthrs_tail, num_workers_with_tail);
#else
    printf(
        "Advisor Warning: TACKLE is compiled to not have hetero workers,"
        " but the current settings leads to hetero workers"
        " and %d cores will not be used.\n",
        nthrs_tail);
#endif
    fflush(0);
  }
  return 0;
}

int affinity_advisor(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts,
                     worker_opts_t *p_workers)
{
  int status;

  if (p_mach_info == NULL || p_tackle_opts == NULL || p_workers == NULL) {
    printf("ERROR: Affinity Advisor initialization failed\n");
    return 1;
  }

  status = initialize_workers(p_mach_info, p_tackle_opts, p_workers);
  if (status) {
    return 0;
  }

  status = populate_workers(p_mach_info, p_tackle_opts, p_workers);
  if (status) {
    return 1;
  }

  show_workers_config(p_tackle_opts, p_workers);

  return 0;
}
