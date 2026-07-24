#set -x
typeset -i BGWRK=${BGWRK:-0}

(( BGWRK > 0)) && {
    bgpids=()
    for ((j=1; j<=BGWRK; j++)); do
	{
	    ((i=0)); while true; do ((i++)); sleep 1; cat /etc/passwd > /tmp/bgwrk.$i.out; done;
	} &
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
sync; sync; sync; sudo  $GDB ./kcuttest $@
for p in "${bgpids[@]}"; do
    echo "killing bgwrk: $p"
    kill -9 $p
done
wait
#sudo systemctl default

