#ifndef TACKLE_H
#define TACKLE_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

#define TACKLE_DEFAULT_NUM_WORKERS (1)
#define TACKLE_DEFAULT_INTER_OP (1)
#define TACKLE_MAX_BUFFER_LENGTH (512)

#define TACKLE_HETERO_WORKERS
#define TACKLE_USE_SHORT_CPU_LIST
#define TACKLE_USE_LOGICAL_CORES_IN_TASKSET
// #define TACKLE_USE_LOGICAL_CORES_IN_OMP

typedef enum { TACKLE, ADVISOR, CHECKER } tackle_mode_t;

typedef struct {
  int num_socks;
  int total_phy_cores;
  int total_log_cores;

  int phy_cores_per_sock;
  int log_cores_per_sock;

  int is_ht;
  int threads_per_core;

  int *core_siblings;

  int num_numa_nodes;
  int **numa_nodes_cpus;
} machine_info_t;

typedef struct {
  int num_workers;
  int inter_op;
  int *omp_num_threads;
  char **omp_affinity;
  char **exec_cmd;
  char **output_files;
  tackle_mode_t mode;
} tackle_opts_t;

typedef struct {
  int num_threads;
  char num_threads_str[TACKLE_MAX_BUFFER_LENGTH];
  char cpus_str[TACKLE_MAX_BUFFER_LENGTH];
  char affinity_str[TACKLE_MAX_BUFFER_LENGTH];
  char numa_str[TACKLE_MAX_BUFFER_LENGTH];
  char *exec_cmd;
  bool skip_launch;
} worker_opts_t;

void set_mach_info(machine_info_t *);
void show_mach_info(machine_info_t *);
void read_env(tackle_opts_t *);
void show_env_info(tackle_opts_t *);
void probe_affinity(int, machine_info_t *, worker_opts_t *, int *);
void show_affinity(machine_info_t *, tackle_opts_t *, worker_opts_t *, int *);
void inspect_affinity(machine_info_t *, tackle_opts_t *, worker_opts_t *,
                      int *);
void dump_affinity_map(machine_info_t *, tackle_opts_t *, worker_opts_t *, int *);
void set_tackle_opts(int, char **, tackle_opts_t *);
int map_cpu_to_sock(int, machine_info_t *);
int map_cpu_to_core(int, machine_info_t *);
int map_core_to_sock(int, machine_info_t *);
int map_cpu_to_numa_node(int, machine_info_t *);
int is_ht_sibling(int, int, machine_info_t *);
int get_ht_sibling(int, machine_info_t *);

int affinity_checker(machine_info_t *, tackle_opts_t *, worker_opts_t *);
int affinity_advisor(machine_info_t *, tackle_opts_t *, worker_opts_t *);

int tackle_driver(machine_info_t *, tackle_opts_t *);
int advisor_driver(machine_info_t *, tackle_opts_t *);
int checker_driver(machine_info_t *, tackle_opts_t *);
#endif
