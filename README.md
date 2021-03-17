# AWSTests

IOR & NPB Setup Instructions

1. Download the tools you'll need:

* IOR: https://sourceforge.net/projects/ior-sio/
* NPB: https://www.nas.nasa.gov/assets/npb/NPB3.3.1.tar.gz

2. Start starcluster

4. Once running, use this script to transfer the tar folders or do it manually:

```bash
#Script for uploading tar files to the master node
IP='YOUR_MASTER_IP_HERE'
scp -i ~/.ssh/AWSKey.pem ./NPB3.3.1.tar.gz youruser@$IP:/home/youruser/
scp -i ~/.ssh/AWSKey.pem ./IOR.tgz youruser@$IP:/home/youruser/
```

5. SSH into your master node and unzip the compressed folders.

```bash
cd /home/yourusername/
tar -xf NPB3.3.1.tar.gz
tar -xf IOR.tgz
```

6. Create the NPB binaries. Note: Make sure you have NPB 3.3 and not 3.4.

```bash
cd ./NPB3.3.1/NPB3.3-MPI/config
cp ./NAS.samples/make.def_sun_mpich ./make.def
cd ..

make EP NPROCS=1 CLASS=A
make EP NPROCS=2 CLASS=A
make EP NPROCS=4 CLASS=A
make EP NPROCS=6 CLASS=A
make EP NPROCS=8 CLASS=A

cd bin
```

7. Upload all ep.A.x files in the bin folder to your github

8. Make the IOR binary

```bash
cd ~/home/youruser/IOR/src/C
make
```

9. Upload the created IOR binary to your github. Now, each time you set up star cluster, just clone your github folder so you don't have to compile everything again.

```bash
cd ~home/youruser/yourgithubfolder
```

10. Create this script for NPB:

```bash
# Usage:   ./test.sh TEST_NUM_OPTION
# Example: ./test.sh 1
# Example: ./test.sh 2

# Command line options: 
# -b is block size of 4M
# -t is transfer size of 4M
# -i is repetition count of 4

DATE=`date "+%b%d"`
mpiexec -host master ep.A.1 > ./Tests/"${DATE} - NPB Test $1 - 1 Node.txt"
mpiexec -host master,node001 ep.A.2 > ./Tests/"${DATE} - NPB Test $1 - 2 Node.txt"
mpiexec -host master,node001,node002,node003 ep.A.4 > ./Tests/"${DATE} - NPB Test $1 - 4 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005 ep.A.6 > ./Tests/"${DATE} - NPB Test $1 - 6 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005,node006,node007 ep.A.8 > ./Tests/"${DATE} - NPB Test $1 - 8 Node.txt"
```


11. Create this script for IOR:

```bash
# Usage:   ./test.sh TEST_NUM_OPTION
# Example: ./test.sh 1
# Example: ./test.sh 2

# Command line options: 
# -b is block size of 4M
# -t is transfer size of 4M
# -i is repetition count of 4

DATE=`date "+%b%d"`
mpiexec -host master IOR -b m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 1 Node.txt"
mpiexec -host master,node001 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 2 Node.txt"
mpiexec -host master,node001,node002,node003 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 4 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 6 Node.txt"
mpiexec -host master,node001,node002,node003,node004,node005,node006,node007 IOR -b 4m -t 4m -i 4 > ./Tests/"${DATE} - IOR Test $1 - 8 Node.txt"
```

12. Run each test.sh script 3 times for the day with the test number as a command line argument.

Example:

* test.sh 1
* test.sh 2
* test.sh 3

13. Update your git repo

```git
git add *
git commit -a -m "Tests for Mon Day"
git push
```
