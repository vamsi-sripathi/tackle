#include "tackle.h"

typedef enum {
  NUM_SOCKETS,
  CORES_PER_SOCKET,
  THREADS_PER_CORE,
  NUM_NUMA_NODES
} lscpu_field_t;

static void get_lscpu_field(lscpu_field_t fname, int *p_dst)
{
  FILE *fp = NULL;
  // 10 chars for INT_MAX and 2 more for potential newline and mandatory string
  // termination char
  char str[12];

  if (fname == NUM_SOCKETS) {
    fp = popen("lscpu | awk '/Socket\\(s\\):/{printf $NF}'", "r");
  } else if (fname == CORES_PER_SOCKET) {
    fp = popen("lscpu | awk '/Core\\(s\\) per socket:/{printf $NF}'", "r");
  } else if (fname == THREADS_PER_CORE) {
    fp = popen("lscpu | awk '/Thread\\(s\\) per core:/{printf $NF}'", "r");
  } else if (fname == NUM_NUMA_NODES) {
    fp = popen("lscpu | awk '/NUMA node\\(s\\):/{printf $NF}'", "r");
  } else {
    printf("ERROR: Unknown lscpu field name\n");
    exit(1);
  }
  if (fp == NULL) {
    perror("popen");
    exit(1);
  }
  if (fgets(str, 12, fp)) {
    *p_dst = atoi(str);
  } else {
    printf("ERROR: Output from lscpu is empty!\n");
    exit(1);
  }
  if (pclose(fp) == -1) {
    perror("pclose");
    exit(1);
  }
}

void set_mach_info(machine_info_t *p_mach_info)
{
  FILE *fp;
  get_lscpu_field(NUM_SOCKETS, &p_mach_info->num_socks);
  get_lscpu_field(CORES_PER_SOCKET, &p_mach_info->phy_cores_per_sock);
  get_lscpu_field(THREADS_PER_CORE, &p_mach_info->threads_per_core);
  get_lscpu_field(NUM_NUMA_NODES, &p_mach_info->num_numa_nodes);

  p_mach_info->is_ht = (p_mach_info->threads_per_core > 1);
  p_mach_info->log_cores_per_sock =
      p_mach_info->phy_cores_per_sock * p_mach_info->threads_per_core;
  p_mach_info->total_phy_cores =
      p_mach_info->num_socks * p_mach_info->phy_cores_per_sock;
  p_mach_info->total_log_cores =
      p_mach_info->num_socks * p_mach_info->log_cores_per_sock;

  p_mach_info->core_siblings =
      (int *)malloc(sizeof(int) * p_mach_info->total_phy_cores *
                    p_mach_info->threads_per_core);
  int *sibling_counter =
      (int *)malloc(sizeof(int) * p_mach_info->total_phy_cores);
  if (p_mach_info->core_siblings == NULL || sibling_counter == NULL) {
    printf("ERROR: Memory allocation failed\n");
    exit(1);
  }

  for (int i = 0; i < p_mach_info->total_phy_cores; i++) {
    sibling_counter[i] = 0;
  }

  fp = popen("lscpu --parse=core,cpu | grep -v \"#\"", "r");
  if (fp == NULL) {
    perror("popen");
    exit(1);
  }

  char line_buffer[TACKLE_MAX_BUFFER_LENGTH];
  char *tok;
  int core_id, sibling_id;
  while (fgets(line_buffer, TACKLE_MAX_BUFFER_LENGTH, fp)) {
    if ((tok = strtok(line_buffer, ",")) != NULL) {
      core_id = atoi(tok);
      if ((tok = strtok(NULL, ",")) != NULL) {
        sibling_id = atoi(tok);
        p_mach_info->core_siblings[(core_id * p_mach_info->threads_per_core) +
                                   sibling_counter[core_id]] = sibling_id;
        sibling_counter[core_id]++;
      }
    }
  }

  free(sibling_counter);
  if (pclose(fp) == -1) {
    perror("pclose");
    exit(1);
  }

  p_mach_info->numa_nodes_cpus =
      (int **)malloc(sizeof(int *) * p_mach_info->num_numa_nodes);
  for (int i = 0; i < p_mach_info->num_numa_nodes; i++) {
    p_mach_info->numa_nodes_cpus[i] =
        (int *)malloc(sizeof(int) * TACKLE_MAX_BUFFER_LENGTH);
  }

  fp = popen(
      "numactl -H | grep \"node [0-9]* cpus:\" | awk -F \":\" '{print $NF}'",
      "r");
  if (fp == NULL) {
    perror("popen");
    exit(1);
  }
  int l_counter = 0;
  int c_counter;
  char *str;
  while (fgets(line_buffer, TACKLE_MAX_BUFFER_LENGTH, fp)) {
    for (str = line_buffer, c_counter = 0; (tok = strtok(str, " ")) != NULL;
         str = NULL,
        p_mach_info->numa_nodes_cpus[l_counter][++c_counter] = atoi(tok)) {
    }
    p_mach_info->numa_nodes_cpus[l_counter][0] = c_counter;
    l_counter++;
  }
  if (pclose(fp) == -1) {
    perror("pclose");
    exit(1);
  }
}

void show_mach_info(machine_info_t *p_mach_info)
{
  printf("Machine topology:\n");
  printf("\tNumber of sockets         = %d\n", p_mach_info->num_socks);
  printf("\tPhysical cores per socket = %d\n", p_mach_info->phy_cores_per_sock);
  printf("\tLogical cores per socket  = %d\n", p_mach_info->log_cores_per_sock);
  printf("\tTotal physical cores      = %d\n", p_mach_info->total_phy_cores);
  printf("\tTotal logical cores       = %d\n", p_mach_info->total_log_cores);
  printf("\tThreads per core          = %d\n", p_mach_info->threads_per_core);
  printf("\tHyper-Threading           = %s\n",
         ((p_mach_info->is_ht) ? "ON" : "OFF"));
  printf("\tNumber of NUMA nodes      = %d\n", p_mach_info->num_numa_nodes);

  for (int i = 0; i < p_mach_info->num_socks; i++) {
    int *sock_siblings =
        p_mach_info->core_siblings + i * p_mach_info->log_cores_per_sock;
    printf("\tSocket-%d CPU(s)         :", i);
    for (int j = 0; j < p_mach_info->phy_cores_per_sock; j++) {
      printf(" %d", sock_siblings[j * p_mach_info->threads_per_core]);
    }
    if (p_mach_info->is_ht) {
      printf("\n\tSocket-%d CPU(s) siblings:", i);
      for (int j = 0; j < p_mach_info->phy_cores_per_sock; j++) {
        for (int k = 1; k < p_mach_info->threads_per_core; k++) {
          printf(" %d", sock_siblings[j * p_mach_info->threads_per_core + k]);
        }
      }
    }
    printf("\n");
  }

  for (int i = 0; i < p_mach_info->num_numa_nodes; i++) {
    printf("\tNUMA node-%d CPU(s)      :", i);
    for (int j = 1; j <= p_mach_info->numa_nodes_cpus[i][0]; j++) {
      printf(" %d", p_mach_info->numa_nodes_cpus[i][j]);
    }
    printf("\n");
  }

  printf("\n");
  fflush(0);
}
