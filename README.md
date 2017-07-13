# OMP Overhead

OpenMP Affinity Verification and Overhead Measurement

This code measures the overhead associated with launching and managing OpenMP threads on a system.
It achieves this by
   1. Measuring the time to execute a serial synthetic work loop;
   2. Measuring the time to execute a parallel version of the same loop, 
      each thread performing the same amount of work as in the 
      single-threaded case. (weak scaling);
   3. Repeating the parallel loop measurement to get a new time that (assuming
      thread pooling) will not have thread creation time in it.

Additionally, the code prints out information for each thread to show affinity information. For each thread this includes,

   * MPI Rank ID
   * Thread ID
   * Total number of threads
   * CPU ID (equivalent to logical hardware thread ID)
   * Physical Core ID
   * NUMA Node ID (equivalent to socket ID for man Intel packages)
   * Node hostname

# Getting the code

```
 % git clone https://github.com/dogunter/omp_overhead
```

This command will check out the source code, including necessary makefiles.

# Requirements

The code requires an installed hwloc package.

https://www.open-mpi.org/projects/hwloc/

# Build Instructions

Assuming the chosen compiler and MPI implementation are in your path, 

```
% make [intel | cray | gnu | pgi]
```

will make a binary corresponding to that compiler, e.g. `omp_overhead.INTEL`

### PGI development has stalled due to Portland Group not releasing a compiler that supports OpenMP 4 at this time.

# Running

The code is typically run in an interactive session. Once proper OMP and other requisite OpenMP environment variables have been set, run as follows,

```
% mpirun -n <# MPI ranks> <other mpirun options> omp_overhead.INTEL <options>
```

where the following options are recognized,

  `--results`  Whether to output timing results (off by default)

  `--csv`      Output in simple csv format (off by default)

# Sample output

Note: Sample output was produced on a test system utilizing Intel Broadwell processors with 2 sockets per package, 18 cores per socket and 2 hardware threads (hyperthreads) per core. These processors have hardware threads numbered 0-17 on the first 18 cores of socket 0, threads 18-35 on socket 1; then threads 36-53 back on socket 0, and threads 54-71 on socket 1.

Example 1: Run 1 MPI rank on 1 socket, 8 OpenMP threads, 1 thread per core,

```
% export OMP_PLACES=cores
% export OMP_PROC_BIND=true
% export OMP_NUM_THREADS=8
% mpirun -n 1 omp_overhead.INTEL --results 
Rank 0, thread 0 of 8 on CPU 0, core 0, NUMA node 0 (kit031.localdomain).
Rank 0, thread 1 of 8 on CPU 3, core 3, NUMA node 0 (kit031.localdomain).
Rank 0, thread 2 of 8 on CPU 5, core 8, NUMA node 0 (kit031.localdomain).
Rank 0, thread 3 of 8 on CPU 7, core 10, NUMA node 0 (kit031.localdomain).
Rank 0, thread 4 of 8 on CPU 9, core 16, NUMA node 0 (kit031.localdomain).
Rank 0, thread 5 of 8 on CPU 12, core 19, NUMA node 0 (kit031.localdomain).
Rank 0, thread 6 of 8 on CPU 14, core 24, NUMA node 0 (kit031.localdomain).
Rank 0, thread 7 of 8 on CPU 16, core 26, NUMA node 0 (kit031.localdomain).
Results (time in seconds)
Rank  Serial   Parallel Overhead nThreads
0:    0.3377   0.3493   0.01162         8
```

Example 2: Run 1 MPI rank on 1 socket, 8 OpenMP threads, 2 threads per core,

```
% export OMP_PLACES=threads
% export OMP_PROC_BIND=close
% mpirun -n 1 omp_overhead.INTEL --results
Rank 0, thread 0 of 8 on CPU 0, core 0, NUMA node 0 (kit031.localdomain).
Rank 0, thread 1 of 8 on CPU 36, core 0, NUMA node 0 (kit031.localdomain).
Rank 0, thread 2 of 8 on CPU 1, core 1, NUMA node 0 (kit031.localdomain).
Rank 0, thread 3 of 8 on CPU 37, core 1, NUMA node 0 (kit031.localdomain).
Rank 0, thread 4 of 8 on CPU 2, core 2, NUMA node 0 (kit031.localdomain).
Rank 0, thread 5 of 8 on CPU 38, core 2, NUMA node 0 (kit031.localdomain).
Rank 0, thread 6 of 8 on CPU 3, core 3, NUMA node 0 (kit031.localdomain).
Rank 0, thread 7 of 8 on CPU 39, core 3, NUMA node 0 (kit031.localdomain).
Results (time in seconds)
Rank  Serial   Parallel Overhead nThreads
0:    0.3125   0.3853   0.07282         8
```

Example 3: Run 1 MPI rank per socket (NUMA region) across both sockets, 36 threads per socket (2 per core), for 72 threads total, using Open-MPI 2.0 launch features,

```
% export OMP_PLACES=threads
% export OMP_PROC_BIND=close
% export OMP_NUM_THREADS=36
% mpirun -n 2 --mca hwloc_base_use_hwthreads_as_cpus true --map-by ppr:1:socket --bind-to socket ./omp_overhead.GNU 
Rank 0, thread 0 of 36 on CPU 0, core 0, NUMA node 0 (kit031.localdomain).
Rank 0, thread 1 of 36 on CPU 1, core 1, NUMA node 0 (kit031.localdomain).
Rank 0, thread 2 of 36 on CPU 2, core 2, NUMA node 0 (kit031.localdomain).
Rank 0, thread 3 of 36 on CPU 3, core 3, NUMA node 0 (kit031.localdomain).
Rank 0, thread 4 of 36 on CPU 4, core 4, NUMA node 0 (kit031.localdomain).
Rank 0, thread 5 of 36 on CPU 5, core 8, NUMA node 0 (kit031.localdomain).
Rank 0, thread 6 of 36 on CPU 6, core 9, NUMA node 0 (kit031.localdomain).
Rank 0, thread 7 of 36 on CPU 7, core 10, NUMA node 0 (kit031.localdomain).
Rank 0, thread 8 of 36 on CPU 8, core 11, NUMA node 0 (kit031.localdomain).
Rank 0, thread 9 of 36 on CPU 9, core 16, NUMA node 0 (kit031.localdomain).
Rank 0, thread 10 of 36 on CPU 10, core 17, NUMA node 0 (kit031.localdomain).
Rank 0, thread 11 of 36 on CPU 11, core 18, NUMA node 0 (kit031.localdomain).
Rank 0, thread 12 of 36 on CPU 12, core 19, NUMA node 0 (kit031.localdomain).
Rank 0, thread 13 of 36 on CPU 13, core 20, NUMA node 0 (kit031.localdomain).
Rank 0, thread 14 of 36 on CPU 14, core 24, NUMA node 0 (kit031.localdomain).
Rank 0, thread 15 of 36 on CPU 15, core 25, NUMA node 0 (kit031.localdomain).
Rank 0, thread 16 of 36 on CPU 16, core 26, NUMA node 0 (kit031.localdomain).
Rank 0, thread 17 of 36 on CPU 17, core 27, NUMA node 0 (kit031.localdomain).
Rank 0, thread 18 of 36 on CPU 36, core 0, NUMA node 0 (kit031.localdomain).
Rank 0, thread 19 of 36 on CPU 37, core 1, NUMA node 0 (kit031.localdomain).
Rank 0, thread 20 of 36 on CPU 38, core 2, NUMA node 0 (kit031.localdomain).
Rank 0, thread 21 of 36 on CPU 39, core 3, NUMA node 0 (kit031.localdomain).
Rank 0, thread 22 of 36 on CPU 40, core 4, NUMA node 0 (kit031.localdomain).
Rank 0, thread 23 of 36 on CPU 41, core 8, NUMA node 0 (kit031.localdomain).
Rank 0, thread 24 of 36 on CPU 42, core 9, NUMA node 0 (kit031.localdomain).
Rank 0, thread 25 of 36 on CPU 43, core 10, NUMA node 0 (kit031.localdomain).
Rank 0, thread 26 of 36 on CPU 44, core 11, NUMA node 0 (kit031.localdomain).
Rank 0, thread 27 of 36 on CPU 45, core 16, NUMA node 0 (kit031.localdomain).
Rank 0, thread 28 of 36 on CPU 46, core 17, NUMA node 0 (kit031.localdomain).
Rank 0, thread 29 of 36 on CPU 47, core 18, NUMA node 0 (kit031.localdomain).
Rank 0, thread 30 of 36 on CPU 48, core 19, NUMA node 0 (kit031.localdomain).
Rank 0, thread 31 of 36 on CPU 49, core 20, NUMA node 0 (kit031.localdomain).
Rank 0, thread 32 of 36 on CPU 50, core 24, NUMA node 0 (kit031.localdomain).
Rank 0, thread 33 of 36 on CPU 51, core 25, NUMA node 0 (kit031.localdomain).
Rank 0, thread 34 of 36 on CPU 52, core 26, NUMA node 0 (kit031.localdomain).
Rank 0, thread 35 of 36 on CPU 53, core 27, NUMA node 0 (kit031.localdomain).
Rank 1, thread 0 of 36 on CPU 18, core 0, NUMA node 1 (kit031.localdomain).
Rank 1, thread 1 of 36 on CPU 19, core 1, NUMA node 1 (kit031.localdomain).
Rank 1, thread 2 of 36 on CPU 20, core 2, NUMA node 1 (kit031.localdomain).
Rank 1, thread 3 of 36 on CPU 21, core 3, NUMA node 1 (kit031.localdomain).
Rank 1, thread 4 of 36 on CPU 22, core 4, NUMA node 1 (kit031.localdomain).
Rank 1, thread 5 of 36 on CPU 23, core 8, NUMA node 1 (kit031.localdomain).
Rank 1, thread 6 of 36 on CPU 24, core 9, NUMA node 1 (kit031.localdomain).
Rank 1, thread 7 of 36 on CPU 25, core 10, NUMA node 1 (kit031.localdomain).
Rank 1, thread 8 of 36 on CPU 26, core 11, NUMA node 1 (kit031.localdomain).
Rank 1, thread 9 of 36 on CPU 27, core 16, NUMA node 1 (kit031.localdomain).
Rank 1, thread 10 of 36 on CPU 28, core 17, NUMA node 1 (kit031.localdomain).
Rank 1, thread 11 of 36 on CPU 29, core 18, NUMA node 1 (kit031.localdomain).
Rank 1, thread 12 of 36 on CPU 30, core 19, NUMA node 1 (kit031.localdomain).
Rank 1, thread 13 of 36 on CPU 31, core 20, NUMA node 1 (kit031.localdomain).
Rank 1, thread 14 of 36 on CPU 32, core 24, NUMA node 1 (kit031.localdomain).
Rank 1, thread 15 of 36 on CPU 33, core 25, NUMA node 1 (kit031.localdomain).
Rank 1, thread 16 of 36 on CPU 34, core 26, NUMA node 1 (kit031.localdomain).
Rank 1, thread 17 of 36 on CPU 35, core 27, NUMA node 1 (kit031.localdomain).
Rank 1, thread 18 of 36 on CPU 54, core 0, NUMA node 1 (kit031.localdomain).
Rank 1, thread 19 of 36 on CPU 55, core 1, NUMA node 1 (kit031.localdomain).
Rank 1, thread 20 of 36 on CPU 56, core 2, NUMA node 1 (kit031.localdomain).
Rank 1, thread 21 of 36 on CPU 57, core 3, NUMA node 1 (kit031.localdomain).
Rank 1, thread 22 of 36 on CPU 58, core 4, NUMA node 1 (kit031.localdomain).
Rank 1, thread 23 of 36 on CPU 59, core 8, NUMA node 1 (kit031.localdomain).
Rank 1, thread 24 of 36 on CPU 60, core 9, NUMA node 1 (kit031.localdomain).
Rank 1, thread 25 of 36 on CPU 61, core 10, NUMA node 1 (kit031.localdomain).
Rank 1, thread 26 of 36 on CPU 62, core 11, NUMA node 1 (kit031.localdomain).
Rank 1, thread 27 of 36 on CPU 63, core 16, NUMA node 1 (kit031.localdomain).
Rank 1, thread 28 of 36 on CPU 64, core 17, NUMA node 1 (kit031.localdomain).
Rank 1, thread 29 of 36 on CPU 65, core 18, NUMA node 1 (kit031.localdomain).
Rank 1, thread 30 of 36 on CPU 66, core 19, NUMA node 1 (kit031.localdomain).
Rank 1, thread 31 of 36 on CPU 67, core 20, NUMA node 1 (kit031.localdomain).
Rank 1, thread 32 of 36 on CPU 68, core 24, NUMA node 1 (kit031.localdomain).
Rank 1, thread 33 of 36 on CPU 69, core 25, NUMA node 1 (kit031.localdomain).
Rank 1, thread 34 of 36 on CPU 70, core 26, NUMA node 1 (kit031.localdomain).
Rank 1, thread 35 of 36 on CPU 71, core 27, NUMA node 1 (kit031.localdomain).
```

# Release

This software has not been approved for open source release.

# Copyright

Copyright (c) 2017, Los Alamos National Security, LLC
All rights reserved.

Copyright 2017. Los Alamos National Security, LLC. This software was produced under U.S. Government contract DE-AC52-06NA25396 for Los Alamos National Laboratory (LANL), which is operated by Los Alamos National Security, LLC for the U.S. Department of Energy. The U.S. Government has rights to use, reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is modified to produce derivative works, such modified software should be clearly marked, so as not to confuse it with the version available from LANL.

Additionally, redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of Los Alamos National Security, LLC, Los Alamos National Laboratory, LANL, the U.S. Government, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
