#!/bin/bash
set -x
typeset -i BGWRK=${BGWRK:-0}
typeset -a kcuttestargs=()
case $1 in
    schedsoft)
	# simple scheduling smoke test
	# sleep seconds = 1 : sleep/blocking test
	# yieldcnt=5 : drives user yield, kernel yield and cond_resched tests
	# preemptcnt=10000000 : ~10M iters ≈ 10–30ms → 3–7 timer ticks at 250Hz
	# force_preempt_full=1 : auto-enables kernel preemption config=full on
	# and restore when done
	kcuttestargs=(1 0 5 0 0 0 0 0 0 0 10000000 1)
	;;
    schedmedium)
	# hammer all paths but don't done create extra background runnable work
	# sleep seconds=2 : sleep/blocking test
	# yieldcnt=1000 : drives user yield, kernel yield and cond_resched tests
	# preemptcnt=500000000 : ~500ms at tight loop rate, many preemptions
	# force_preempt_full=1 : auto-enables kernel preemption config=full on
	# and restore when done
	kcuttestargs=(2 0 1000 0 0 0 0 0 0 0 500000000 1)
    ;;
    schedhard)
	# hammer all paths with background runnable processes ensured
	# sleep seconds=2 : sleep/blocking test
	# yieldcnt=1000 : drives user yield, kernel yield and cond_resched tests
	# preemptcnt=500000000 : ~500ms at tight loop rate, many preemptions
	# force_preempt_full=1 : auto-enables kernel preemption config=full on
	# and restore when done
	((BGWRK==0)) && BGWRK=$(($(nproc)))
	kcuttestargs=(2 0 1000 0 0 0 0 0 0 0 500000000 1)
    ;;
    *)
	kcuttestargs=($@)
    ;;
esac 

# backgound cpu centric works -- infinte loop of computing sin
# no io to avoid blocking 
(( BGWRK > 0)) && {
    bgpids=()
    for ((j=1; j<=BGWRK; j++)); do
	bc -l <<< "while (1) { x=s(1.5) }" & 
	bgpids+=($!)
    done
}
(( BGWRK )) && echo "STARTED BACKGROUND BUSY LOOPS: ${bgpids[@]}"


#sudo LD_DEBUG=all LD_BIND_NOW=1 LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./main

# BIND_NOW kludge until we get page fault mitigated
#sudo LD_BIND_NOW=1 LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./main

#sync; sync; sync; sudo LD_DEBUG=all LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./main

#sudo kill -SIGUSR1 1
[[ -n $GDB ]] && GDB="$GDB -args"
sync; sync; sync; sudo  $GDB ./kcuttest ${kcuttestargs[@]}
for p in "${bgpids[@]}"; do
    echo "killing bgwrk: $p"
    kill -9 $p
done
wait
#sudo systemctl default

