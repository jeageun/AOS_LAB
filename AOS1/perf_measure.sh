for opt_random_access in 0 1
do
    for ANON in 0 1 
    do
        for POPULATE in 0 1
        do
            for SHARED in  1 
            do
                for MSET in 0 1 
                do
                    rm perf.o
                    CFLAGS="-DMMAP_ALLOC -DFORK "
                    if [ $ANON -eq 1 ]
                    then
                      CFLAGS=$CFLAGS"-DANON " 
                    fi
                    if [ $POPULATE -eq 1 ]
                    then
                      CFLAGS=$CFLAGS"-DPOPULATE " 
                    fi
                    if [ $SHARED -eq 1 ]
                    then
                      CFLAGS=$CFLAGS"-DSHARED " 
                    fi
                    if [ $MSET -eq 1 ]
                    then
                      CFLAGS=$CFLAGS"-DMSET " 
                    fi
                    if [ $opt_random_access -eq 1 ]
                    then
                        CFLAGS=$CFLAGS"-Dopt_random_access=1 "
                    else
                        CFLAGS=$CFLAGS"-Dopt_random_access=0 "
                    fi
                    echo $CFLAGS 
                    gcc $CFLAGS open_read_perf.c -o perf.o
                    
                    for j in {1..5}
                    do
                        echo $j
                        ./perf.o >> output.txt
                        sleep 2
                    done
                
                done
            done
        done
    done
done
