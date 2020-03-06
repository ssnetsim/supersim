#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import tempfile

try:
  import termcolor
  can_color = True
except ImportError:
  can_color = False

def good(s):
  if can_color:
    print('  ' + termcolor.colored(s, 'green'))
  else:
    print('  ' + s)

def bad(s):
  if can_color:
    print('  ' + termcolor.colored(s, 'red'))
  else:
    print('  ' + s)

def main(args):
  if args.check:
    subprocess.check_call('valgrind -h', shell=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  # check if binary exists
  assert os.path.exists(args.supersim)
  assert os.path.exists(args.config_file)

  # generate command
  cmd = '{0} {1}'.format(args.supersim, args.config_file)
  if args.check:
    cmd = ('valgrind --log-fd=1 --leak-check=full --show-reachable=yes '
           '--track-origins=yes --track-fds=yes {0}'.format(cmd))

  # add a log file to the command
  log = tempfile.mkstemp()[1]
  cmd = '{0} 2>&1 | tee {1}'.format(cmd, log)

  # run tasks
  print('running simulation')
  proc = subprocess.run(cmd, shell=True)
  print('done')

  # check return code
  assert proc.returncode == 0, ('Return code was non-zero: {}'
                                .format(proc.returncode))

  # check output for failures
  error = False
  print('analyzing {0} output'.format(args.config_file))

  # read in text
  with open(log, 'r') as fd:
    lines = fd.readlines();
  os.remove(log)

  # analyze output
  sim_complete = False
  for idx, line in enumerate(lines):

    if line.find('Simulation complete') >= 0:
      sim_complete = True
    if args.check:
      if (line.find('Open file descriptor') >= 0 and
          lines[idx+1].find('inherited from parent') < 0):
        error = True
        bad('open file descriptor')
      if line.find('blocks are definitely lost') >= 0:
        error = True
        bad('definitely lost memory')
      if line.find('blocks are indirectly lost') >= 0:
        error = True
        bad('indirectly lost memory')
      if (line.find('blocks are still reachable') >= 0 and
          # TODO(nic): REMOVE ME WHEN G++ STOPS SUCKING
          not line.find('72,704 bytes') >= 0):
        error = True
        bad('still reachable memory')
      if line.find('depends on uninitialised value') >= 0:
        error = True
        bad('depends on uninitialised value')

  if not sim_complete:
    error = True;
    bad('no "Simulation complete" message')

  # show status
  if not error:
    good('passed all tests')

  return -1 if error else 0

if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('config_file', type=str,
                  help='config file to run and check')
  ap.add_argument('-m', '--check', action='store_true',
                  help='Use valgrind to check the memory validity')
  ap.add_argument('-s', '--supersim', type=str, default='./bazel-bin/supersim',
                  help='supersim binary to use')
  args = ap.parse_args()
  print(args)
  sys.exit(main(args))
