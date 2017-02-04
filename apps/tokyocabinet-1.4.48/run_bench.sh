for rsize in 64 128 256 512 1024 4096
do
    for percent in 25 50 75 100
    do
        ./eval_tokyo.py -r1 -t8 -q100000 -s$rsize -w$percent -d0
    done
done
