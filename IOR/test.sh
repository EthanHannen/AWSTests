#MAKE SURE TO SUPPLY THE TEST NUMBER AS AN ARGUMENT WHEN RUNNING THE SCRIPT
# ./test.sh 1
# ./test.sh 2
# ./test.sh 3

DATE=`date "+%b-%d"`

mpiexec -host master IOR -b 1m -t 1m > ./Tests/"${DATE} | Test $1 | 1 Node.txt"
mpiexec -host master,node001 IOR -b 2m -t 2m > ./Tests/"${DATE} | Test $1 | 2 Node.txt"
mpiexec -host master,node001,node002,node003 IOR b -b 4m -t 4m > ./Tests/"${DATE} | Test $1 | 4 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005 IOR -b 6m -t 6m > ./Tests/"${DATE} | Test $1 | 6 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005,node006,node007 IOR -b 8m -t 8m > ./Tests/"${DATE} | Test $1 | 8 Node.txt"
