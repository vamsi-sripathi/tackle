#include "tackle.h"

static void prep_for_launch(tackle_opts_t *p_tackle_opts,
                              worker_opts_t *p_workers)
{
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    p_workers[i].skip_launch = false;

    if (!strlen(p_workers[i].num_threads_str) || !strlen(p_workers[i].affinity_str)
        || !strlen(p_tackle_opts->exec_cmd[i])) {
      p_workers[i].skip_launch = true;
    }

    char tmp_buffer[TACKLE_MAX_BUFFER_LENGTH];
    if (strlen(p_workers[i].cpus_str) && strlen(p_workers[i].numa_str)) {
      int nbytes;
      strcpy(tmp_buffer, p_tackle_opts->exec_cmd[i]);
      nbytes =
        sprintf(p_tackle_opts->exec_cmd[i], "taskset -c %s numactl %s ",
                p_workers[i].cpus_str, p_workers[i].numa_str);

      strcpy(p_tackle_opts->exec_cmd[i] + nbytes, tmp_buffer);
    }
    // redirect stderr, stdout to file
    sprintf(tmp_buffer, " &> %s", p_tackle_opts->output_files[i]);
    strcpy(p_tackle_opts->exec_cmd[i] + strlen(p_tackle_opts->exec_cmd[i]),
           tmp_buffer);
    p_workers[i].exec_cmd = p_tackle_opts->exec_cmd[i];
  }
}

static void show_launch_config(tackle_opts_t *p_tackle_opts,
                               worker_opts_t *p_workers)
{
  printf("\nLauncher configuration:\n");
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    if (!p_workers[i].skip_launch) {
      printf("WID[%d]:\n", i);
      printf("\tOMP_NUM_THREADS = %s\n", p_workers[i].num_threads_str);
      printf("\tKMP_AFFINITY    = %s\n", p_workers[i].affinity_str);
      printf("\tExec program    : %s\n", p_workers[i].exec_cmd);
      printf("\tOutput file     : %s\n", p_tackle_opts->output_files[i]);
    } else {
      printf("Launcher Warning: WID[%d] has empty launch parameters!\n", i);
    }
    fflush(0);
  }
}

int tackle_driver(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts)
{
  worker_opts_t *p_workers = NULL;
  FILE **fp_list = NULL;
  int num_workers, status;

  num_workers = p_tackle_opts->num_workers;
  p_workers = (worker_opts_t *)malloc(sizeof(worker_opts_t) * num_workers);
  if (p_workers == NULL) {
    printf("ERROR: Tackle Memory allocation failed\n");
    exit(1);
  }

  status = affinity_advisor(p_mach_info, p_tackle_opts, p_workers);
  if (status) {
    printf("ERROR: Affinity Advisor failed\n");
    exit(1);
  }

  status = affinity_checker(p_mach_info, p_tackle_opts, p_workers);
  if (status) {
    printf("ERROR: Affinity Checker failed\n");
    exit(1);
  }

  prep_for_launch(p_tackle_opts, p_workers);
  show_launch_config(p_tackle_opts, p_workers);

  fp_list = (FILE **)malloc(sizeof(FILE *) * num_workers);
  if (fp_list == NULL) {
    printf("\nERROR: malloc failed for fp_list\n");
    exit(1);
  }

  // Launch workers
  for (int i = 0; i < num_workers; i++) {
    if (!p_workers[i].skip_launch) {
      setenv("OMP_NUM_THREADS", p_workers[i].num_threads_str, 1);
      setenv("KMP_AFFINITY", p_workers[i].affinity_str, 1);

      fp_list[i] = popen(p_workers[i].exec_cmd, "r");

      if (fp_list[i] == NULL) {
        perror("popen");
        exit(1);
      }
    } else {
      printf(
          "Warning: Skipping launching of WID[%d] since it has empty "
          "parameters!\n",
          i);
    }
  }

  for (int i = 0; i < num_workers; i++) {
    if (!p_workers[i].skip_launch) {
      status = pclose(fp_list[i]);
      if (status == -1) {
        perror("pclose");
      }
    }
  }

  free(fp_list);

  return 0;
}
