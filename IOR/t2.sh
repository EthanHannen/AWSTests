  
DATE=`date "+%b%d"`
mpiexec -host master,node001 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 2 Node.txt"
