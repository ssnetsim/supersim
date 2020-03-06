#!/usr/bin/env python3

import argparse
import math
import os
import psutil
import subprocess
import sys

try:
  import termcolor
  canColor = True
except ImportError:
  canColor = False

def logFile(jsonFile):
  base = os.path.splitext(os.path.basename(jsonFile))[0]
  return '/tmp/supersimtest_{0}'.format(base)

def good(s):
  if canColor:
    print('  ' + termcolor.colored(s, 'green'))
  else:
    print('  ' + s)

def bad(s):
  if canColor:
    print('  ' + termcolor.colored(s, 'red'))
  else:
    print('  ' + s)

def main(args):
  if args.check:
    subprocess.check_call('valgrind -h', shell=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  # check if binary exists
  assert os.path.exists('supersim')

  # generate command
  cmd = './supersim {0}'.format(args.config_file)
  if args.check:
    cmd = ('valgrind --log-fd=1 --leak-check=full --show-reachable=yes '
           '--track-origins=yes --track-fds=yes {0}'.format(cmd))
  log = logFile(args.config_file)
  try:
    os.remove(log)
  except OSError:
    pass
  cmd = '{0} 2>&1 | tee {1}'.format(cmd, log)

  # run tasks
  print('running simulation')
  subprocess.run(cmd, shell=True)
  print('done')

  # check output for failures
  anyError = False
  error = False
  print('analyzing {0} output'.format(args.config_file))

  # read in text
  log = logFile(args.config_file)
  with open(log, 'r') as fd:
    lines = fd.readlines();

  # analyze output
  simComplete = False
  for idx, line in enumerate(lines):

    if line.find('Simulation complete') >= 0:
      simComplete = True
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

  if not simComplete:
    error = True;
    bad('no "Simulation complete" message')

  # show status
  if error:
    anyError = True
  else:
    good('passed all tests')

  return 0 if not anyError else -1

if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('config_file', type=str,
                  help='config file to run and check')
  ap.add_argument('-c', '--cpus', type=int,
                  help='number of CPUs to utilize')
  ap.add_argument('-r', '--mem', type=float,
                  help='amount of memory to utilize (in GiB)')
  ap.add_argument('-m', '--check', action='store_true',
                  help='Use valgrind to check the memory validity')
  args = ap.parse_args()
  print(args)
  sys.exit(main(args))
