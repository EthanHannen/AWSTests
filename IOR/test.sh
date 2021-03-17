DATE=`date "+%b%d"`
mpiexec -host master IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 1 Node.txt"
mpiexec -host master,node001 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 2 Node.txt"
mpiexec -host master,node001,node002,node003 IOR b -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 4 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 6 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005,node006,node007 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 8 Node.txt"
