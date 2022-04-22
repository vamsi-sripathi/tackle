Tackle is a set of thread affinity tools namely, Advisor, Checker and Launcher.

## Checker:

In this mode, Tackle inspects the OpenMP affinity specified by user and reports potential anomalies based on CPU topology. It achieves this by spawning OpenMP Parallel regions to probe thread affinity. The following classes of affinity defects are identified -
- Missing affinity for a given thread
- Threads are floating among core siblings AKA Hyper-Threading cores
- Over-subscription of cpus
- Threads crossing socket boundary

Reports the Thread->Core affinity map as GraphViz dot output file (use, ```dot -Tpng tackle_affinity_map.gv > output.png``` to view it as image)

## Advisor:

For a given number of workers or/and threads, recommends the following -
- Number of threads
- Optimal OMP affinity based on machine topology
- NUMA domain based on thread placement

## Launcher:

This takes as input the number of workers, number of OpenMP threads/worker, path to execution command/script and optional log file names. The launcher would then start executing the target app (as multiple instances) with optimal affinity settings

## Building Tackle
You will need the Intel C Compiler and Intel OpenMP runtime library. Run "make" and that will produce a binary called, *tackle*

## Tester
There is a bash shell script in /tester directory that checks the various modes supported with different worker configs.

## List of options supported 

```
[user@linux]$ ./tackle -h
tackle [options] -- command

In the default mode, Tackle will take the command specified and launch it according to the options specified
Tackle has two other modes -- advisor, checker. In these modes, Tackle will only provide recommendations (advisor mode)
or report anomalies (checker mode) based on user input and machine topology

The following options are available depending upon the mode specified:
-m, --mode         : Either of advisor or checker.
-w, --num_workers  : Specifies the number of instances of command to launch. Default is 1.
-t, --num_thrs     : Specifies the number of threads to be used in each instance.
                     For number of workers > 1 and if the number of threads to
                     be used is not same among the workers, you can specify multiple
                     values, each delimited by a semi-colon(;). For eg. "5;8;10"
                     If the number of threads to be used is same across the workers,
                     you can just specify one value and it would be used for all workers
-i, --inter_op     : Not applicable for checker mode.
                     Number of processes a worker will spawn. Default is 1.
-a, --affinity     : Applicable only for checker mode.
                     Specifies the KMP_AFFINITY option for each worker delimited by semi-colon(;)
-e, --exec_cmd     : Not applicable for advisor and checker modes.
                     Specifies the command to launch for workers with different
                     exec commands, you can specify them delimited by a semi-colon(;)
                     If the same exec command is used for all workers, you have a choice to skip this
                     option and specify the exec command after the -- token. In all modes, no command will be launched by default.
-o, --out_file     : Not applicable for advisor and checker modes.
                     Specifies the path to the file to which worker stdout and stderr are written
                     If specified, a worker suffix(.w[0,1,..]) will be added to the end of the file
                     name. The default is to write stdout and stderr of each worker to file
                     named "tackle-worker-[0,1..].log" in the current directory.
```
