#!/bin/bash
# Sergio Pino

if [ ! $# == 5 ]; then
	echo "Usage: $0 executable(main) group_size(512) step(0.01) CSNR(15) folder(exp_p1)"
	exit
fi

#RMANAGER=srun -N1 --gres=gpu:1
RMANAGER=""

src=( 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 )
chn=( 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 )
cm=$1
gs=$2
st=$3
csnr=$4
folder=$5

if [ ! -d "$folder" ]; then
	echo "Creating the folder "$folder
	mkdir $folder
fi

for i in 0 1 2 3 4 5 6 7 8 9 10 11 ; do	
	echo "--Processing src"${src[i]}" and noise"${chn[i]}
	$RMANAGER ./$cm $gs $st src${src[i]}.txt ${src[i]} noise${chn[i]}.txt ${chn[i]} $csnr > $folder"/"${src[i]}.txt
done  
