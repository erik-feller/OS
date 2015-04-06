#!/usr/bin/bash

TIMEFORM=$'wall=%e user=%U system=%S CPU=%P i-switched=%c v-switched=%w'

PROGS=( pi-sched rw multi-sched)
FORKS=( 10 100 1000 )
SCHEDULERS=( SCHED_OTHER SCHED_FIFO SCHED_RR )
for a in ${PROGS[@]}; do
		for b in ${SCHEDULERS[@]};do
				for c in ${FORKS[@]};do
						echo $a $b $c; time ./$a $b $c 1 > outhar 2 > results
				done
		done
done
