#MAKE SURE TO SUPPLY THE TEST NUMBER AS AN ARGUMENT WHEN RUNNING THE SCRIPT
# ./test.sh 1
# ./test.sh 2
# ./test.sh 3

DATE=`date "+%b%d"`

mpiexec -host master ep.A.1 > ./Tests/"${DATE} - Test $1 - 1 Node.txt"
mpiexec -host master,node001 ep.A.2 > ./Tests/"${DATE} - Test $1 - 2 Node.txt"
mpiexec -host master,node001,node002,node003 ep.A.4 > ./Tests/"${DATE} - Test $1 - 4 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005 ep.A.6 > ./Tests/"${DATE} - Test $1 - 6 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005,node006,node007 ep.A.8 > ./Tests/"${DATE} - Test $1 - 8 Node.txt"
