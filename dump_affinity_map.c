#include "tackle.h"

void dump_affinity_map(machine_info_t *p_mach_info, tackle_opts_t *p_tackle_opts,
                       worker_opts_t *p_workers, int *p_global_affinity)
{
  int num_workers = p_tackle_opts->num_workers;
  int ncores = p_mach_info->total_log_cores;
  int worker_nthrs;
  int *p_ithr_affinity;

  FILE *fp = fopen("tackle_affinity_map.gv", "w");
  if (fp == NULL) {
    printf("ERROR: fopen failed for writing gv file\n");
    exit(1);
  }

  fprintf(fp, "#\n");
  fprintf(fp, "# Run dot -Tpng -O tackle_affinity_map.gv to visualize "
          "the thread -> cpu affinity map\n");
  fprintf(fp, "#\n");
  fprintf(fp, "digraph affinity_map {\n");
  fprintf(fp, "\tnewrank=true;\n");

  for (int i = 0; i < p_mach_info->num_socks; i++) {
    int *sock_siblings =
        p_mach_info->core_siblings + i * p_mach_info->log_cores_per_sock;
    fprintf(fp, "\tsubgraph cluster_sock_%d {\n", i);
    fprintf(fp, "\t\tlabel=\"Socket-%d\";\n", i);
    fprintf(fp,
            "\t\tstyle=filled; color=lightblue; node "
            "[shape=box,style=filled,color=white];\n");
    for (int j = 0; j < p_mach_info->phy_cores_per_sock; j += 1) {
      fprintf(fp, "\t\tsubgraph cluster_core%d {\n",
              i * p_mach_info->phy_cores_per_sock + j);
      fprintf(fp, "\t\t\tlabel=\"CORE-%d\";\n",
              i * p_mach_info->phy_cores_per_sock + j);
      fprintf(fp,
              "\t\t\tstyle=filled; color=violet; node "
              "[shape=box,style=filled,color=white];\n");
      fprintf(fp, "\t\t\trank=same;\n");
      for (int l = 0; l < p_mach_info->threads_per_core; l++) {
        if (l == p_mach_info->threads_per_core - 1) {
          fprintf(fp, "\"CPU-%d\" ",
                  sock_siblings[j * p_mach_info->threads_per_core + l]);
        } else {
          fprintf(fp, "\t\t\t\"CPU-%d\" -> ",
                  sock_siblings[j * p_mach_info->threads_per_core + l]);
        }
      }
      fprintf(fp, "[style=invis];\n");
      fprintf(fp, "\t\t}\n");  // close phy.core
    }
    for (int j = 0; j < p_mach_info->phy_cores_per_sock;
         j += p_mach_info->phy_cores_per_sock / 2) {
      for (int jj = j; jj < j + p_mach_info->phy_cores_per_sock / 2 &&
                       jj < p_mach_info->phy_cores_per_sock;
           jj++) {
        if (jj == j) {
          fprintf(fp, "\t\t");
        }
        if (jj == j + (p_mach_info->phy_cores_per_sock / 2) - 1) {
          fprintf(fp, "\"CPU-%d\" ",
                  sock_siblings[jj * p_mach_info->threads_per_core + 0]);
        } else {
          fprintf(fp, "\"CPU-%d\" -> ",
                  sock_siblings[jj * p_mach_info->threads_per_core + 0]);
        }
      }
      fprintf(fp, "[style=invis];\n");
    }
    fprintf(fp, "\t}\n");  // close socket
  }

  for (int w = 0, worker_offset = 0; w < num_workers; w++) {
    worker_nthrs = p_workers[w].num_threads;
    worker_offset += ((w) ? p_workers[w - 1].num_threads * ncores : 0);
    fprintf(fp, "\tsubgraph cluster_wid_%d {\n", w);
    fprintf(fp, "\t\tstyle=filled; color=forestgreen;\n");
    fprintf(fp, "\t\tnode [shape=box,style=filled,color=grey];\n");
    fprintf(fp,
            "\t\tedge[arrowhead=vee, arrowtail=inv, arrowsize=.7, color=maroon, "
            "fontsize=10, fontcolor=navy];\n");
    fprintf(fp, "\t\tlabel=\"WID[%d]\";\n", w);
    for (int t = 0; t < worker_nthrs; t++) {
      p_ithr_affinity = p_global_affinity + worker_offset + t * ncores;
      for (int c = 0; c < ncores; c++) {
        if (p_ithr_affinity[c] != -1) {
          fprintf(fp, "\t\t\"[%d][%d]\" -> \"CPU-%d\";\n", w, t,
                  p_ithr_affinity[c]);
        }
      }
    }
    fprintf(fp, "\t}\n");  // close worker
  }

  fprintf(fp, "}\n");  // close graph
  fclose(fp);
}
