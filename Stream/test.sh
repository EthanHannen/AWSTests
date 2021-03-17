mpiexec -host master ./stream_mpi>Test1
mpiexec -host master,node001 ./stream_mpi>Test2
mpiexec -host master,node001,node002,node003 ./stream_mpi>Test4
mpiexec -host master,node001,node002,node003,node004,node005 ./stream_mpi>Test6
mpiexec -host master,node001,node002,node003,node004,node005,node006,node007 ./stream_mpi>Test8