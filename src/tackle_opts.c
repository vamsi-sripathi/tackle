#include <getopt.h>
#include "tackle.h"

static void show_help()
{
  printf("tackle [options] -- command\n\n");
  printf(
      "In the default mode, Tackle will take the command specified and launch "
      "it according to the options specified\n");
  printf(
      "Tackle has two other modes -- advisor, checker. In these modes, Tackle "
      "will only provide recommendations (advisor mode)\n"
      "or report anomalies (checker mode) based on user input and machine "
      "topology\n\n");
  printf(
      "The following options are available depending upon the mode "
      "specified:\n");
  printf("-m, --mode         : Either of advisor or checker.\n");
  printf(
      "-w, --num_workers  : Specifies the number of instances of command to "
      "launch. Default is 1.\n");
  printf(
      "-t, --num_thrs     : Specifies the number of threads to be used in each "
      "instance.\n"
      "                     For number of workers > 1 and if the number of "
      "threads to\n"
      "                     be used is not same among the workers, you can "
      "specify multiple\n"
      "                     values, each delimited by a semi-colon(;). For eg. "
      "\"5;8;10\"\n"
      "                     If the number of threads to be used is same across "
      "the workers,\n"
      "                     you can just specify one value and it would be "
      "used for all workers\n");
  printf(
      "-i, --inter_op     : Not applicable for checker mode.\n"
      "                     Number of processes a worker will spawn. Default "
      "is 1.\n");
  printf(
      "-a, --affinity     : Applicable only for checker mode.\n"
      "                     Specifies the KMP_AFFINITY option for each worker "
      "delimited by semi-colon(;)\n");
  printf(
      "-e, --exec_cmd     : Not applicable for advisor and checker modes.\n"
      "                     Specifies the command to launch for workers "
      "with different\n"
      "                     exec commands, you can specify them delimited by a "
      "semi-colon(;)\n"
      "                     If the same exec command is used for all workers, "
      "you have a choice to skip this\n"
      "                     option and specify the exec command after the -- "
      "token. In all modes, no command will be launched by default.\n");
  printf(
      "-o, --out_file     : Not applicable for advisor and checker modes.\n"
      "                     Specifies the path to the file to which worker "
      "stdout and stderr are written\n"
      "                     If specified, a worker suffix(.w[0,1,..]) will be "
      "added to the end of the file\n"
      "                     name. The default is to write stdout and "
      "stderr of each worker to file\n"
      "                     named \"tackle-worker-[0,1..].log\" in the current "
      "directory.\n");
  fflush(0);
}

static void show_tackle_config(tackle_opts_t *p_tackle_opts)
{
  printf("User specified Tackle config:\n");
  printf("Number of workers = %d\n", p_tackle_opts->num_workers);
  for (int i = 0; i < p_tackle_opts->num_workers; i++) {
    printf("\tWORKER-%d:\n", i);
    printf("\tNumber of threads  = %d\n", p_tackle_opts->omp_num_threads[i]);
    printf("\tNumber of inter_op = %d\n", p_tackle_opts->inter_op);
    printf("\tAffinity           = %s\n", p_tackle_opts->omp_affinity[i]);
    printf("\tExec               = %s\n", p_tackle_opts->exec_cmd[i]);
    printf("\tOutput file        = %s\n\n", p_tackle_opts->output_files[i]);
  }
  fflush(0);
}

void set_tackle_opts(int argc, char **argv, tackle_opts_t *p_tackle_opts)
{
  int cursor, key;
  char *w_val, *t_val, *e_val, *o_val, *a_val, *m_val, *i_val;
  w_val = t_val = e_val = o_val = a_val = m_val = i_val = NULL;
  struct option cli_opts[] = {{"num_workers", required_argument, NULL, 'w'},
                              {"num_thrs", required_argument, NULL, 't'},
                              {"affinity", required_argument, NULL, 'a'},
                              {"exec_cmd", required_argument, NULL, 'e'},
                              {"out_file", required_argument, NULL, 'o'},
                              {"mode", required_argument, NULL, 'm'},
                              {"inter_op", required_argument, NULL, 'i'},
                              {"help", no_argument, NULL, 'h'},
                              {0, 0, 0, 0}};

  while ((key = getopt_long(argc, argv, "-m:w:t:a:e:i:o:h", cli_opts,
                            &cursor)) != -1) {
    switch (key) {
      case 'w':
        w_val = optarg;
        break;
      case 't':
        t_val = optarg;
        break;
      case 'e':
        e_val = optarg;
        break;
      case 'o':
        o_val = optarg;
        break;
      case 'a':
        a_val = optarg;
        break;
      case 'm':
        m_val = optarg;
        break;
      case 'i':
        i_val = optarg;
        break;
      case 'h':
        show_help();
        exit(0);
      case '\1':
        show_help();
        exit(1);
      case '?':
        show_help();
        exit(1);
    }
  }
  p_tackle_opts->num_workers =
      ((w_val) ? atoi(w_val) : TACKLE_DEFAULT_NUM_WORKERS);
  p_tackle_opts->inter_op = ((i_val) ? atoi(i_val) : TACKLE_DEFAULT_INTER_OP);

  if (p_tackle_opts->num_workers < 1) {
    printf("ERROR: Number of workers should be >= 1\n");
    exit(1);
  }
  if (p_tackle_opts->inter_op < 1) {
    printf("ERROR: Number of inter-op threads should be >= 1\n");
    exit(1);
  }

  if (!m_val) {
    p_tackle_opts->mode = TACKLE;
  } else if (strcmp(m_val, "advisor") == 0) {
    p_tackle_opts->mode = ADVISOR;
  } else if (strcmp(m_val, "checker") == 0) {
    p_tackle_opts->mode = CHECKER;
  } else {
    printf(
        "ERROR: Valid values for -m|--mode flag are checker or advisor, "
        "unknown option specified -- %s\n", 
        m_val);
    exit(1);
  }

  int num_workers = p_tackle_opts->num_workers;

  p_tackle_opts->omp_num_threads = (int *)malloc(sizeof(int) * num_workers);
  p_tackle_opts->omp_affinity = (char **)malloc(sizeof(char *) * num_workers);
  p_tackle_opts->exec_cmd = (char **)malloc(sizeof(char *) * num_workers);
  p_tackle_opts->output_files = (char **)malloc(sizeof(char *) * num_workers);
  for (int i = 0; i < num_workers; i++) {
    p_tackle_opts->exec_cmd[i] =
        (char *)malloc(sizeof(char) * TACKLE_MAX_BUFFER_LENGTH);
    p_tackle_opts->output_files[i] =
        (char *)malloc(sizeof(char) * TACKLE_MAX_BUFFER_LENGTH);
  }

  char *str, *token;
  int count;

  for (int i = 0; i < num_workers; i++) {
    p_tackle_opts->omp_num_threads[i] = 0;
    p_tackle_opts->omp_affinity[i] = NULL;
    p_tackle_opts->exec_cmd[i][0] = '\0';
    if (o_val) {
      sprintf(p_tackle_opts->output_files[i], "%s.w%d", o_val, i);
    } else {
      sprintf(p_tackle_opts->output_files[i], "tackle-worker-%d.log", i);
    }
  }

  if (t_val) {
    for (str = t_val, count = 0; ((token = strtok(str, ";")) != NULL) && (count < num_workers);
         str = NULL) {
      p_tackle_opts->omp_num_threads[count] = atoi(token);
      if (p_tackle_opts->omp_num_threads[count] < 1) {
        printf("ERROR: Number of threads should be >= 1\n");
        exit(1);
      }
      count++;
    }
    if (count == 1) {
      for (int i = 1; i < num_workers; i++) {
        p_tackle_opts->omp_num_threads[i] = p_tackle_opts->omp_num_threads[0];
      }
    }
  }

  if (e_val) {
    for (str = e_val, count = 0; ((token = strtok(str, ";")) != NULL) && (count < num_workers);
         str = NULL) {
      strcpy(p_tackle_opts->exec_cmd[count++], token);
    }
    if (count == 1) {
      for (int i = 1; i < num_workers; i++) {
        strcpy(p_tackle_opts->exec_cmd[i], p_tackle_opts->exec_cmd[0]);
      }
    }
  }

  if (a_val) {
    for (str = a_val, count = 0; ((token = strtok(str, ";")) != NULL) && (count < num_workers);
         str = NULL) {
      p_tackle_opts->omp_affinity[count++] = token;
    }
    if (count == 1) {
      for (int i = 1; i < num_workers; i++) {
        p_tackle_opts->omp_affinity[i] = p_tackle_opts->omp_affinity[0];
      }
    }
  }

  for (int i = optind, nbytes = 0, len = 0; i < argc; i++) {
    len += strlen(argv[i]) + 1;
    if (len > TACKLE_MAX_BUFFER_LENGTH) {
      printf("ERROR: Exec command is greater than allowed max buffer size\n");
      exit(1);
    }
    nbytes += sprintf(p_tackle_opts->exec_cmd[0] + nbytes, "%s ", argv[i]);
  }
  for (int i = 1; i < num_workers; i++) {
    strcpy(p_tackle_opts->exec_cmd[i], p_tackle_opts->exec_cmd[0]);
  }

  /* show_tackle_config(p_tackle_opts); */
}
