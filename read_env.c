#include "tackle.h"

void read_env(tackle_opts_t *p_tackle_opts)
{
  char *tackle_num_workers_str = getenv("TACKLE_NUM_WORKERS");
  if (tackle_num_workers_str) {
    p_tackle_opts->num_workers = atoi(tackle_num_workers_str);
  } else {
    p_tackle_opts->num_workers = TACKLE_DEFAULT_NUM_WORKERS;
  }

  if (p_tackle_opts->num_workers < 1) {
    printf("ERROR: Number of workers must be a positive number\n");
    exit(1);
  }

  char exec_cmd_w[TACKLE_MAX_BUFFER_LENGTH];
  char *tmp = NULL;
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    // TACKLE_W[0..n]_EXEC takes predence for multi-worker scenario.
    // If user did not set TACKLE_W[0..n]_EXEC, then check for
    // shared TACKLE_EXEC var across workers.
    sprintf(exec_cmd_w, "TACKLE_W%d_EXEC", i);
    tmp = getenv(exec_cmd_w);
    if (tmp) {
      strcpy(p_tackle_opts->exec_cmd[i], tmp);
    } else {
      tmp = getenv("TACKLE_EXEC");
      if (tmp) {
        strcpy(p_tackle_opts->exec_cmd[i], tmp);
      } else {
        // user did not set TACKLE_EXEC or TACKLE_W[]EXEC
        // so, set the varible to NULL and catch this condition
        // in the launcher module since this variable is not
        // mandatory for TAA or TAC
        *p_tackle_opts->exec_cmd[i] = '\0';
      }
    }
  }

  char omp_nthrs_w[TACKLE_MAX_BUFFER_LENGTH];
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    sprintf(omp_nthrs_w, "TACKLE_W%d_OMP_NUM_THREADS", i);
    // TACKLE_W[0..n]_OMP_NUM_THREADS takes predence for multi-worker scenario.
    // If user did not set TACKLE_W[0..n]_OMP_NUM_THREADS, then check for
    // shared OMP_NUM_THREADS var across workers.
    if (getenv(omp_nthrs_w)) {
      p_tackle_opts->omp_num_threads[i] = atoi(getenv(omp_nthrs_w));
    } else if (getenv("OMP_NUM_THREADS")) {
      p_tackle_opts->omp_num_threads[i] = atoi(getenv("OMP_NUM_THREADS"));
    } else {
      p_tackle_opts->omp_num_threads[i] = 0;
    }
  }

  char kmp_affinity_w[TACKLE_MAX_BUFFER_LENGTH];
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    // TACKLE_W[0..n]_KMP_AFFINITY takes predence for multi-worker scenario.
    // If user did not set TACKLE_W[0..n]_KMP_AFFINITY, then check for
    // shared KMP_AFFINITY var across workers.
    sprintf(kmp_affinity_w, "TACKLE_W%d_KMP_AFFINITY", i);
    p_tackle_opts->omp_affinity[i] = getenv(kmp_affinity_w);
    if (p_tackle_opts->omp_affinity[i] == NULL) {
      p_tackle_opts->omp_affinity[i] = getenv("KMP_AFFINITY");
    }
  }

  char out_file_w[TACKLE_MAX_BUFFER_LENGTH];
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    // TACKLE_W[0..n]_OUT_FILE takes predence for multi-worker scenario.
    // If user did not set TACKLE_W[0..n]_OUT_FILE, then check for
    // shared OUT_FILE var across workers.
    sprintf(out_file_w, "TACKLE_W%d_OUT_FILE", i);
    tmp = getenv(out_file_w);
    if (tmp) {
      strcpy(p_tackle_opts->output_files[i], tmp);
    } else {
      tmp = getenv("TACKLE_OUT_FILE");
      if (tmp) {
        strcpy(p_tackle_opts->output_files[i], tmp);
      } else {
        // user did not set TACKLE_OUT_FILE or TACKLE_W[]_OUT_FILE
        // so set it to default for all workers
        for (int j = 0; j < p_tackle_opts->num_workers; j++) {
          sprintf(p_tackle_opts->output_files[j], "tackle-worker-%d.log", j);
        }
        break;
      }
    }

    if (tmp != NULL && p_tackle_opts->num_workers > 1) {
      char outfile_full[TACKLE_MAX_BUFFER_LENGTH];
      char outfile_base[TACKLE_MAX_BUFFER_LENGTH];
      char outfile_ext[TACKLE_MAX_BUFFER_LENGTH];
      char cmd[TACKLE_MAX_BUFFER_LENGTH];
      FILE *fp;
      sprintf(cmd, "dirname %s", p_tackle_opts->output_files[i]);
      fp = popen(cmd, "r");
      fscanf(fp, "%s", outfile_full);
      if (pclose(fp) == -1) {
        printf("\npopen failed for dirname\n");
        fflush(0);
        exit(1);
      }
      sprintf(cmd, "basename %s | sed \"s/\\./ /1\"",
              p_tackle_opts->output_files[i]);
      fp = popen(cmd, "r");
      fscanf(fp, "%s", outfile_base);
      fscanf(fp, "%s", outfile_ext);
      if (pclose(fp) == -1) {
        printf("\npopen failed for dirname\n");
        fflush(0);
        exit(1);
      }
      sprintf(outfile_full + strlen(outfile_full), "/%s-worker-%d.%s",
              outfile_base, i, outfile_ext);
      strcpy(p_tackle_opts->output_files[i], outfile_full);
    }
  }
}

void show_env_info(tackle_opts_t *p_tackle_opts)
{
  int num_vars_read = 0;
  printf("User defined environment variables:\n");

  if (p_tackle_opts->num_workers > 1) {
    num_vars_read++;
    printf("\tTACKLE_NUM_WORKERS = %d\n", p_tackle_opts->num_workers);
  }

  char omp_nthrs_w[TACKLE_MAX_BUFFER_LENGTH];
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    if (i == 0) {
      if (getenv("OMP_NUM_THREADS")) {
        num_vars_read++;
        printf("\tOMP_NUM_THREADS = %s\n", getenv("OMP_NUM_THREADS"));
      }
    }
    sprintf(omp_nthrs_w, "TACKLE_W%d_OMP_NUM_THREADS", i);
    if (getenv(omp_nthrs_w)) {
      num_vars_read++;
      printf("\tTACKLE_W%d_OMP_NUM_THREADS = %s\n", i, getenv(omp_nthrs_w));
    }
  }

  char kmp_affinity_w[TACKLE_MAX_BUFFER_LENGTH];
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    if (i == 0) {
      if (getenv("KMP_AFFINITY")) {
        num_vars_read++;
        printf("\tKMP_AFFINITY = %s\n", getenv("KMP_AFFINITY"));
      }
    }
    sprintf(kmp_affinity_w, "TACKLE_W%d_KMP_AFFINITY", i);
    if (getenv(kmp_affinity_w)) {
      num_vars_read++;
      printf("\tTACKLE_W%d_KMP_AFFINITY = %s\n", i, getenv(kmp_affinity_w));
    }
  }

  char exec_cmd_w[TACKLE_MAX_BUFFER_LENGTH];
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    if (i == 0) {
      if (getenv("TACKLE_EXEC")) {
        num_vars_read++;
        printf("\tTACKLE_EXEC = %s\n", getenv("TACKLE_EXEC"));
      }
    }
    sprintf(exec_cmd_w, "TACKLE_W%d_EXEC", i);
    if (getenv(exec_cmd_w)) {
      num_vars_read++;
      printf("\tTACKLE_W%d_EXEC = %s\n", i, getenv(exec_cmd_w));
    }
  }

  char outfile_w[TACKLE_MAX_BUFFER_LENGTH];
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    if (i == 0) {
      if (getenv("TACKLE_OUT_FILE")) {
        num_vars_read++;
        printf("\tTACKLE_OUT_FILE = %s\n", getenv("TACKLE_OUT_FILE"));
      }
    }
    sprintf(outfile_w, "TACKLE_W%d_OUT_FILE", i);
    if (getenv(outfile_w)) {
      num_vars_read++;
      printf("\tTACKLE_W%d_OUT_FILE = %s\n", i, getenv(outfile_w));
    }
  }

  if (!num_vars_read) {
    printf("\tNone. Defaulting to number of workers = 1");
  }
  printf("\n");
  fflush(0);
}
