# Parameter optimization for joint source-channel coding using OpenCL
Project 2: CISC879  
Sergio Pino  
University of Delaware

For running the experiments you can use either experiments_serial.sh or experiments_parallel.sh.

For both scripts the parameters are the same, however the group_size has no meaning nor use in the serial experiments. Actually, I'm not use it yet since I set the group_size(local_size) to be the result of getting the CL_DEVICE_MAX_WORK_GROUP_SIZE.

> Usage: ./experiments_x.sh executable group_size step CSNR folder

The script will run several experiments with different data sizes. The size ranges from 1024 (2^10) elements to 1048576 (2^20). The results will be stored in the "folder" path passed as argument into the script (if the folder doesn't exist it will create it for you).

The data sizes are ( 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 ). These values are hard coded in the experiments script.

##Serial

1. compile:
> make mainser

2. Run the serial experiments:
> ./experiments_serial.sh mainser 512 0.05 15 exp_serial

## Parallel

There are two version for the parallel computation named Parallel and Parallel2. The script experiments_parallel.sh includes the scheduling using srun -N1 --gres=gpu:1.

1. compile:
> (Parallel 1): make mainp1  
> (Parallel 2): make mainp2

2. Run the parallel experiments:
> (Parallel 1): ./experiments_parallel.sh mainp1 512 0.05 15 exp_p1  
> (Parallel 2): ./experiments_parallel.sh mainp2 512 0.05 15 exp_p2

## Report
For more information you can refer to the [Report.pdf](Report.pdf)