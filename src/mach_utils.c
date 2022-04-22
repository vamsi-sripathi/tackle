#include "tackle.h"

int map_cpu_to_core(int cpu_id, machine_info_t *p_mach_info)
{
  for (int p = 0; p < p_mach_info->total_phy_cores; p++) {
    for (int l = 0; l < p_mach_info->threads_per_core; l++) {
      if (p_mach_info->core_siblings[p * p_mach_info->threads_per_core + l] ==
          cpu_id) {
        return p;
      }
    }
  }
  return -1;
}

int map_core_to_sock(int core_id, machine_info_t *p_mach_info)
{
  for (int i = 0; i < p_mach_info->num_socks; i++) {
    int *sock_siblings =
        p_mach_info->core_siblings + i * p_mach_info->log_cores_per_sock;
    for (int j = 0; j < p_mach_info->phy_cores_per_sock; j++) {
      if (sock_siblings[j * p_mach_info->threads_per_core] == core_id) {
        return i;
      }
    }
  }
  return -1;
}

int map_cpu_to_sock(int cpu_id, machine_info_t *p_mach_info)
{
  return (map_core_to_sock(map_cpu_to_core(cpu_id, p_mach_info), p_mach_info));
}

int is_ht_sibling(int core_id, int cpu_id, machine_info_t *p_mach_info)
{
  int *core_siblings =
      p_mach_info->core_siblings + core_id * p_mach_info->threads_per_core;
  for (int l = 0; l < p_mach_info->threads_per_core; l++) {
    if (core_siblings[l] == cpu_id) {
      return 1;
    }
  }
  return 0;
}

int get_ht_sibling(int core_id, machine_info_t *p_mach_info)
{
  if (p_mach_info->is_ht) {
    return (p_mach_info->core_siblings[core_id * p_mach_info->threads_per_core + 1]);
  } else {
    return core_id;
  }
}

int map_cpu_to_numa_node(int cpu_id, machine_info_t *p_mach_info)
{
  for (int i = 0; i < p_mach_info->num_numa_nodes; i++) {
    for (int j = 1; j <= p_mach_info->numa_nodes_cpus[i][0]; j++) {
      if (p_mach_info->numa_nodes_cpus[i][j] == cpu_id) {
        return i;
      }
    }
  }
  return -1;
}
