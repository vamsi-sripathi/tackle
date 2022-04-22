#include "tackle.h"

int main(int argc, char **argv)
{
  tackle_opts_t tackle_opts;
  machine_info_t mach_info;

  set_tackle_opts(argc, argv, &tackle_opts);
  set_mach_info(&mach_info);
  show_mach_info(&mach_info);

  if (tackle_opts.mode == TACKLE) {
    tackle_driver(&mach_info, &tackle_opts);
  } else if (tackle_opts.mode == ADVISOR) {
    advisor_driver(&mach_info, &tackle_opts);
  } else if (tackle_opts.mode == CHECKER) {
    checker_driver(&mach_info, &tackle_opts);
  }

  return 0;
}
