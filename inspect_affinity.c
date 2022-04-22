#include "tackle.h"

static int check_socket_crossing(int wid, machine_info_t *p_mach_info,
                                 worker_opts_t *p_workers,
                                 int *p_worker_affinity)
{
  int nthrs = p_workers[wid].num_threads;
  int *thread_to_sock_list = (int *)malloc(sizeof(int) * nthrs);
  int ncores = p_mach_info->total_log_cores;
  int tid;
  int is_socket_crossing = 0;

  for (int ithr = 0; ithr < nthrs; ithr++) {
    // Caution: this function is only called when no floating threads are found
    // So, break when you encounter the first pinned cpu
    /* get_pinned_cpu(ithr, ncores, p_worker_affinity); */
    for (int j = 0; j < ncores; j++) {
      if (p_worker_affinity[ithr * ncores + j] != -1) {
        tid = p_worker_affinity[ithr * ncores + j];
        break;
      }
    }
    thread_to_sock_list[ithr] = map_cpu_to_sock(tid, p_mach_info);
  }
  for (int ithr = 1, sock = thread_to_sock_list[0]; ithr < nthrs; ithr++) {
    if (sock != thread_to_sock_list[ithr]) {
      is_socket_crossing = 1;
      break;
    }
  }

  if (is_socket_crossing) {
    int *is_sock_visited = (int *)malloc(sizeof(int) * p_mach_info->num_socks);
    for (int i = 0; i < p_mach_info->num_socks; i++) {
      is_sock_visited[i] = 0;
    }

    printf(
        "[ISSUE DETECTED]: Socket Crossing - WID[%d] threads are crossing CPU "
        "sockets:",
        wid);
    for (int ithr = 0; ithr < nthrs; ithr++) {
      if (!is_sock_visited[thread_to_sock_list[ithr]]) {
        printf(" %d", thread_to_sock_list[ithr]);
        is_sock_visited[thread_to_sock_list[ithr]]++;
      }
    }
    printf("\n");
    free(is_sock_visited);
  }

  free(thread_to_sock_list);
  return is_socket_crossing;
}

static int check_floating_thread(int wid, int ithr, machine_info_t *p_mach_info,
                                 int *p_ithr_affinity)
{
  int ncores = p_mach_info->total_log_cores;
  int core_id;
  int count = 0;
  int *thread_to_core_list =
      (int *)malloc(sizeof(int) * p_mach_info->total_log_cores);
  int *thread_list = (int *)malloc(sizeof(int) * p_mach_info->total_log_cores);

  for (int c = 0; c < ncores; c++) {
    if (p_ithr_affinity[c] != -1) {
      core_id = map_cpu_to_core(p_ithr_affinity[c], p_mach_info);
      if (core_id < 0) {
        printf("\nThread to core mapping failed\n");
        break;
      }
      thread_to_core_list[count] = core_id;
      thread_list[count] = p_ithr_affinity[c];
      count++;
    }
  }

  // If thread is found on only 1 cpu, then this thread is not floating, so
  // return false
  if (count == 1) {
    return 0;
  }

  // Check if thread is floating on all cores aka no affinity case
  if (count == ncores) {
    // If thread is found on logical. count of cpus, then check if they fall on
    // all available phy. cpu
    int landed_cores = 0;
    for (int p = 0; p < p_mach_info->total_phy_cores; p++) {
      for (int i = 0; i < count; i++) {
        if (thread_to_core_list[i] == p) {
          landed_cores++;
          break;
        }
      }
    }
    if (landed_cores == p_mach_info->total_phy_cores) {
      printf(
          "[ISSUE DETECTED]: Missing affinity - WID[%d]TID[%d] has no affinity "
          "and is floating on all %d cores!\n",
          wid, ithr, landed_cores);
      return 1;
    }
  }

  // Check for HT siblings
  int ht_siblings = 0;
  for (int i = 1; i < count; i++) {
    if (is_ht_sibling(thread_to_core_list[0], thread_list[i], p_mach_info)) {
      ht_siblings++;
    }
  }
  if (ht_siblings == count - 1) {
    printf(
        "[ISSUE DETECTED]: Floating thread among core siblings - "
        "WID[%d]TID[%d] thread is floating among core-%d siblings:",
        wid, ithr, thread_to_core_list[0]);
    for (int i = 0; i < count; i++) {
      printf(" %d", thread_list[i]);
    }
    printf("\n");
  } else {
    printf(
        "[ISSUE DETECTED]: Floating thread across cpus - WID[%d]TID[%d] thread "
        "is floating on cpus:",
        wid, ithr);
    for (int i = 0; i < count; i++) {
      printf(" %d", thread_list[i]);
    }
    printf("\n");
  }

  free(thread_to_core_list);
  free(thread_list);

  return 1;
}

static int check_oversubscription(int wid, machine_info_t *p_mach_info,
                                  worker_opts_t *p_workers,
                                  int *p_worker_affinity)
{
  int desired_phy_cores;
  int *p_ithr_affinity;
  int core_id;
  int uniq_phy_cores = 0;
  int worker_nthrs = p_workers[wid].num_threads;
  int ncores = p_mach_info->total_log_cores;
  int *visited_core_id_list =
      (int *)malloc(sizeof(int) * p_mach_info->total_phy_cores);

  desired_phy_cores = worker_nthrs;

  for (int i = 0; i < p_mach_info->total_phy_cores; i++) {
    visited_core_id_list[i] = 0;
  }

  // find uniq phy.cores for each thread
  for (int t = 0; t < worker_nthrs; t++) {
    p_ithr_affinity = p_worker_affinity + t * ncores;
    for (int c = 0; c < ncores; c++) {
      if (p_ithr_affinity[c] != -1) {
        core_id = map_cpu_to_core(p_ithr_affinity[c], p_mach_info);
        if (core_id < 0) {
          printf("\nThread to core mapping failed\n");
          break;
        }
        if (!visited_core_id_list[core_id]) {
          visited_core_id_list[core_id] = 1;
          uniq_phy_cores++;
        }
      }
    }
  }

  free(visited_core_id_list);

  if (uniq_phy_cores < desired_phy_cores) {
    printf(
        "[ISSUE DETECTED]: Over-subscription - WID[%d] requested number of "
        "threads %d are mapped to fewer number of physical cores %d\n",
        wid, desired_phy_cores, uniq_phy_cores);
    return 1;
  }
  return 0;
}

static int get_pinned_core(int ncores, int *p_ithr_affinity)
{
  for (int i = 0; i < ncores; i++) {
    if (p_ithr_affinity[i] != -1) {
      return p_ithr_affinity[i];
    }
  }
  return -1;
}

void inspect_affinity(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts,
                      worker_opts_t *p_workers, int *p_global_affinity)
{
  int *floating_workers, *oversubscribed_workers, *socket_crossing_workers;
  int *p_ithr_affinity, *p_worker_affinity, *core_counter;
  int ncores = p_mach_info->total_log_cores;
  int num_workers = p_tackle_opts->num_workers;

  floating_workers = (int *)malloc(sizeof(int) * num_workers);
  oversubscribed_workers = (int *)malloc(sizeof(int) * num_workers);
  socket_crossing_workers = (int *)malloc(sizeof(int) * num_workers);
  for (int w = 0; w < num_workers; w++) {
    floating_workers[w] = 0;
    oversubscribed_workers[w] = 0;
    socket_crossing_workers[w] = 0;
  }

  // Global core counter for all workers
  core_counter = (int *)malloc(sizeof(int) * ncores);
  for (int c = 0; c < ncores; c++) {
    core_counter[c] = 0;
  }

  printf("\nChecking affinity:\n");

  for (int w = 0, worker_offset = 0; w < num_workers; w++) {
    worker_offset += ((w) ? p_workers[w - 1].num_threads * ncores : 0);

    // Check for floating threads
    for (int t = 0; t < p_workers[w].num_threads; t++) {
      p_ithr_affinity = p_global_affinity + worker_offset + t * ncores;
      if (check_floating_thread(w, t, p_mach_info, p_ithr_affinity)) {
        floating_workers[w]++;
      } else {
        // if thread is not floating, then increment the global core counter
        // this is needed to catch cases where workers are pinned, but using
        // same cpus between them.
        core_counter[get_pinned_core(ncores, p_ithr_affinity)]++;
      }
    }

    // Check for case when workers threads are occupying fewer phy cores than
    // total number of threads in the worker
    p_worker_affinity = p_global_affinity + worker_offset;
    oversubscribed_workers[w] +=
        check_oversubscription(w, p_mach_info, p_workers, p_worker_affinity);

    // Check socket crossing only for non-floating workers
    if (!floating_workers[w]) {
      // don't check socket crossing for workers whose num threads are greater
      // than sock cores
      if (p_workers[w].num_threads <= p_mach_info->phy_cores_per_sock) {
        socket_crossing_workers[w] +=
            check_socket_crossing(w, p_mach_info, p_workers, p_worker_affinity);
      }
    }
  }

  printf("Checker Summary:\n");
  for (int w = 0; w < num_workers; w++) {
    if (floating_workers[w]) {
      printf(
          "\tWID[%d] threads are not affinitized and are floating on multiple "
          "cpus\n",
          w);
      fflush(0);
    }
    if (oversubscribed_workers[w]) {
      printf("\tWID[%d] affinity settings lead to cpu over-subscription\n", w);
      fflush(0);
    }
    if (socket_crossing_workers[w]) {
      printf("\tWID[%d] threads are crossing system sockets\n", w);
      fflush(0);
    }
    if (!floating_workers[w] && !oversubscribed_workers[w] &&
        !socket_crossing_workers[w]) {
      printf(
          "\tWID[%d] Checker did not detect any affinity defects for this "
          "worker threads\n",
          w);
      fflush(0);
    }
  }

  for (int w = 0, break_out = 0; w < num_workers && !break_out; w++) {
    if (!floating_workers[w] && !oversubscribed_workers[w]) {
      /* !socket_crossing_workers[w]) { */
      for (int i = 0; i < ncores; i++) {
        if (core_counter[i] > 1) {
          printf(
              "\tSystem is over-subscribed as multiple"
              " workers are pinned to same cpus:");
          fflush(0);
          break_out++;
          break;
        }
      }
      for (int i = 0; i < ncores; i++) {
        if (core_counter[i] > 1) {
          printf(" %d", i);
          fflush(0);
        }
      }
    }
  }
  printf("\n");

  free(floating_workers);
  free(oversubscribed_workers);
  free(socket_crossing_workers);
  free(core_counter);
}
