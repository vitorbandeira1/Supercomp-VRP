#!/bin/bash
#SBATCH --job-name=n_insertion_VRP
#SBATCH --output=n_insertion_result.txt        
#SBATCH --partition=espec            
#SBATCH --nodes=2                     
#SBATCH --ntasks-per-node=16   
#SBATCH --cpus-per-task=1     
#SBATCH --mem-per-cpu=200M 

# Calculate total number of MPI processes to start
total_procs=$((SLURM_NTASKS_PER_NODE * SLURM_NNODES))

# Run the program using mpirun or mpiexec; adjust depending on your MPI implementation
mpirun -np $total_procs ./ninsertion