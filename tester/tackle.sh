#!/bin/bash

set -x 

TACKLE_SRC=../src/ && cd ${TACKLE_SRC} && make && TACKLE_BINARY=./tackle

function test_checker_mode()
{
  ${TACKLE_BINARY} --mode checker
  ${TACKLE_BINARY} --mode checker --num_thrs 10 --affinity compact
  ${TACKLE_BINARY} --mode checker --num_thrs 10 --affinity granularity=fine,explicit,proclist=[0-4]
  ${TACKLE_BINARY} --mode checker --num_thrs 10 --affinity granularity=fine,explicit,proclist=[15-24]
  ${TACKLE_BINARY} --mode checker --num_thrs 10 --affinity granularity=fine,compact,1,0
  ${TACKLE_BINARY} --mode checker --num_workers 4 --num_thrs 10
  ${TACKLE_BINARY} --mode checker --num_workers 4 --num_thrs 10 --affinity compact
  ${TACKLE_BINARY} --mode checker --num_workers 4 --num_thrs 10 --affinity granularity=fine,compact,1,0
  ${TACKLE_BINARY} --mode checker --num_workers 4 --num_thrs 10 --affinity granularity=fine,explicit,proclist=[0-9]
  ${TACKLE_BINARY} --mode checker --num_workers 4 --num_thrs 10 --affinity "granularity=fine,explicit,proclist=[0-9];granularity=fine,explicit,proclist=[10-19];granularity=fine,explicit,proclist=[20-29];granularity=fine,explicit,proclist=[30-39]"
}

function test_advisor_mode()
{
  ${TACKLE_BINARY} --mode advisor
  ${TACKLE_BINARY} --mode advisor --num_workers 4
  ${TACKLE_BINARY} --mode advisor --num_workers 5 --num_thrs 8
  ${TACKLE_BINARY} --mode advisor --num_workers 4 --num_thrs "4;10;12;8"
  ${TACKLE_BINARY} --mode advisor --num_workers 5 --num_thrs 8 --inter_op 2
  ${TACKLE_BINARY} --mode advisor --num_workers 7
  ${TACKLE_BINARY} --mode advisor --num_workers 10 --num_thrs 8
}

function test_full_tackle()
{
  ${TACKLE_BINARY}
  ${TACKLE_BINARY} --num_workers 4 -- "env | grep -E 'OMP|KMP'"
  ${TACKLE_BINARY} --num_workers 4 --inter_op 2
  ${TACKLE_BINARY} --num_workers 10
}

test_checker_mode
test_advisor_mode
test_full_tackle
