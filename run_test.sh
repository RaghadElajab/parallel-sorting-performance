#!/bin/bash -l

#SBATCH --job-name=sortBench
#SBATCH --nodes=1

# Reserve 10 CPUs on one node for every array task
#SBATCH --ntasks=10
#SBATCH --cpus-per-task=1

#SBATCH --partition=prod
#SBATCH --time=3-00:00:00
#SBATCH --account=teach0031

# Run one method at a time
#SBATCH --array=0-3%1

#SBATCH --output=sort_%A_%a.out
#SBATCH --error=sort_%A_%a.err

cd "$SLURM_SUBMIT_DIR" || exit 1

# Stop immediately if compilation or execution fails
set -e

# Load the C compiler and MPI
module purge
module load spack/1.0.2
module load gcc/14.1.0-b6muzd6
module load openmpi/5.0.8-sytl4md

echo "========================================"
echo "Main job ID: $SLURM_ARRAY_JOB_ID"
echo "Array task: $SLURM_ARRAY_TASK_ID"
echo "Node: $SLURMD_NODENAME"
echo "Started: $(date)"
echo "========================================"


case "$SLURM_ARRAY_TASK_ID" in

    0)
        echo "Running Sequential C"

        # -x c is needed because your filename uses uppercase .C
        gcc -x c -O2 \
            sequential_sort.C \
            -o sequential_sort

        srun \
            --ntasks=1 \
            --cpus-per-task=1 \
            --cpu-bind=cores \
            ./sequential_sort
        ;;

    1)
        echo "Running Pthreads with 10 threads"

        gcc -O2 -pthread \
            multithreading.c \
            -o multithreading

        srun \
            --ntasks=1 \
            --cpus-per-task=10 \
            --cpu-bind=cores \
            env SLURM_CPUS_PER_TASK=10 \
            ./multithreading
        ;;

    2)
        echo "Running OpenMP with 10 threads"

        gcc -O2 -fopenmp \
            openMP.c \
            -o openMP

        srun \
            --ntasks=1 \
            --cpus-per-task=10 \
            --cpu-bind=cores \
            env SLURM_CPUS_PER_TASK=10 \
                OMP_NUM_THREADS=10 \
            ./openMP
        ;;

    3)
        echo "Running MPI with 10 processes"

        mpicc -O2 \
            mpi.c \
            -o mpi

        srun \
            --ntasks=10 \
            --cpus-per-task=1 \
            --cpu-bind=cores \
            ./mpi
        ;;

    *)
        echo "Unknown array task"
        exit 1
        ;;

esac


echo "========================================"
echo "Task completed: $(date)"
echo "========================================"