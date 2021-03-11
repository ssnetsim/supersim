#!/usr/bin/env python3

import argparse
import os
import sssweep
import taskrun


###############################################################################
# PROGRAM ARGUMENTS

ap = argparse.ArgumentParser()
ap.add_argument('directory', type=str, default='output',
                help='the output directory')
ap.add_argument('supersim', type=str,
                help='supersim binary')
ap.add_argument('ssparse', type=str,
                help='ssparse binary')
ap.add_argument('transient', type=str,
                help='transient script')
ap.add_argument('settings', type=str,
                help='settings file to use (fattree_iq_blast.json)')
ap.add_argument('-g', '--granularity', type=float, default=6.0,
                help='the granularity of the injection rate sweeps')
args = ap.parse_args()

###############################################################################
# SETUP

# get the current amount of resources
cpus = os.cpu_count()
mem = taskrun.MemoryResource.current_available_memory_gib();

# build the task manager
rm = taskrun.ResourceManager(taskrun.CounterResource('cpus', 9999, cpus),
                             taskrun.MemoryResource('mem', 9999, mem))
cob = taskrun.FileCleanupObserver()
vob = taskrun.VerboseObserver()
tm = taskrun.TaskManager(
  resource_manager=rm,
  observers=[cob, vob],
  failure_mode=taskrun.FailureMode.AGGRESSIVE_FAIL)

# create task and resources function
def create_task(tm, name, cmd, console_out, task_type, config):
  task = taskrun.ProcessTask(tm, name, cmd)
  if console_out:
    task.stdout_file = console_out
    task.stderr_file = console_out
  if task_type == 'sim':
    task.resources = {'cpus': 1, 'mem': 0.5}
  elif task_type == 'parse':
    task.resources = {'cpus': 1, 'mem': 0.5}
  else:
    task.resources = {'cpus': 1, 'mem': 1.1}
  return task

# create sweeper
sw = sssweep.Sweeper(
  args.supersim, args.settings, args.ssparse, args.transient,
  create_task, args.directory, sim=True,
  latency_scalar=0.001, latency_units='ns')

###############################################################################
# SIMULATION VARIABLES

# routing variable
routing_algorithms = ['flow_hash', 'flow_cache', 'oblivious', 'adaptive']
def set_routing_algorithm(ra, config):
  if ra == 'flow_hash':
    sel, red = 'flow_hash', 'all_minimal'
  elif ra == 'flow_cache':
    sel, red = 'flow_cache', 'all_minimal'
  elif ra == 'oblivious':
    sel, red = 'all', 'all_minimal'
  elif ra == 'adaptive':
    sel, red = 'all', 'least_congested_minimal'
  else:
    assert False
  return ('/network/protocol_classes/0/routing/selection=string={0} '
          '/network/protocol_classes/0/routing/reduction/algorithm=string={1}'
          .format(sel, red))
sw.add_variable('Routing Algorithm', 'RA', routing_algorithms,
                set_routing_algorithm, compare=True)

# loads
start = 0.0
stop = 100.0
step = args.granularity
def set_load(ld, config):
  ld /= 100.0
  cmd = ('/workload/applications/0/blast_terminal/request_injection_rate='
         'float={0} '
         '/workload/applications/0/blast_terminal/enable_responses=bool=false '
         .format(0.001 if ld == 0.0 else ld))
  return cmd
sw.add_loads('Load', 'LD', start, stop, step, set_load)

###############################################################################
# PLOTS

# ssparse
sw.add_plot('load-latency-compare', 'ssparse')
sw.add_plot('load-latency', 'ssparse')
sw.add_plot('load-percent-minimal', 'ssparse', yauto_frame=0.2)
sw.add_plot('load-average-hops', 'ssparse', yauto_frame=0.2)
sw.add_plot('load-rate-percent', 'ssparse')

sw.add_plot('latency-pdf', 'ssparse2', ['+app=0'])
sw.add_plot('latency-percentile', 'ssparse2', ['+app=0'])
sw.add_plot('latency-cdf', 'ssparse2', ['+app=0'])
sw.add_plot('time-latency-scatter', 'ssparse2', ['+app=0'])

# straight through
sw.add_plot('load-rate', 'none')

# transient
sw.add_plot('time-percent-minimal', 'transient', yauto_frame=0.2)
sw.add_plot('time-average-hops', 'transient', non_minimal='n')
sw.add_plot('time-latency', 'transient')

###############################################################################
# CREATE AND RUN

# auto-add tasks to the task manager
sw.create_tasks(tm)

# run the tasks!
tm.run_tasks()
