#!/bin/sh

mkdir -p zzuf_crashes

for s in $(seq 1 10000000); do
    zzuf -s $s -r 0.01:0.5 < in/Pass.3mf > fuzz.3mf

    ./test_normal fuzz.3mf > /dev/null 2>&1
    status=$?

    if [ $status -ge 128 ]; then
        signal=$((status - 128))
        cp fuzz.3mf zzuf_crashes/crash-$s-sig$signal.3mf
        echo "Crash at seed $s (signal $signal)"
    else
        echo "OK seed $s (exit $status)"
    fi
done
