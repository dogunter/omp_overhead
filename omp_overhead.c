//
// omp_overhead.c
//
// Measures OMP overhead and speedup, also reports thread affinity status.
// 1. Measure the time to execute a serial loop
// 2. Measure the time to execute a parallel loop, each thread doing
//    the same amount of work as in the single thread case. (weak scaling)
// 3. Repeat parallel loop measurement to get a new time that (assuming
//    thread pooling) will not have thread creation time in it.
// 4. Reports task and thread affinity information.
//      MPI Rank ID
//      CPU ID, same as hardware thread ID, same as PU# from lstopo output
//      physical core ID
//      NUMA node ID, same as socket ID
//      hostname
//
//  Options:
//  --results Whether to output timing results (off by default)
//  --csv Output in simple csv format (off by default)

#include "hwloc.h"
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include "omp_common.h"

// Get a physical core ID from a CPU ID
int core_from_cpu(hwloc_topology_t, int);

// rdtscp:  Read Time-Stamp Counter and Processor ID IA assembly instruction
//
// It is the easist way to determine which numa region (socket) and which 
// core (actually hardware thread ID) a process is running on. The value of 
// the timestamp register is stored into the EDX and EAX registers; the 
// value of the CPU id info is stored into the ECX register 
//
// This is a generic TSC reader if the compiler does not provide an intrinsic for it.
// (The Intel intrisic is simply __rdtscp.)
// Read the discussion here as to how this came about.
// https://software.intel.com/en-us/forums/intel-isa-extensions/topic/280440 (17 Feb 2014)
//
unsigned long generic_rdtscp(int *core, int *sckt)
{
  unsigned int a, d, c;

  __asm__ volatile("rdtscp" : "=a" (a), "=d" (d), "=c" (c));

  *sckt = (c & 0xFFF000)>>12;  // socket info is the higher order bits of the ECX register
  *core = c & 0xFFF;           // Hardware thread ID is the lower order bits of the ECX

  return ( (unsigned long)a ) | ( ((unsigned long)d ) << 32 );;
}

//
// workloop is a simple pseudo work loop
// Compile it as unoptimized since its basic purpose is to provide
// a known delay.
// Sadly, this only works for Gnu compilers,
//     void __attribute__((optimize("O0"))) workloop(int);
//
void workloop(int);

int main(int argc, char** argv) {

   int i, rank, total_ranks;
   int opt = 0;
   int longIndex = 0;
   int workIters, loopReps;
   int tid, nThreads;

   unsigned long int tscValue;
   int cpu_id, core_id, numa_node;

   hwloc_topology_t topology;

   double startTime;
   double sTime; // Serial loop time
   double pTime; // Parallel loop time

   char hnbuf[64]; // holds the hostname (node ID)

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &total_ranks);

   // Create the topology
   hwloc_topology_init(&topology);
   hwloc_topology_load(topology);

   // Parse options
   //  --work-iters <integer>> - The number of iterations for workloop()
   //  --loop-reps <integer> - The number of iterations for the outer loop

   if (rank == 0 ) {
      int long_index =0;
      workIters = WORKITERS;
      loopReps = LOOPREPS;
      while ((opt = getopt_long(argc, argv,"l:w:", longOpts, &long_index )) != -1) {
         switch (opt) {
            case 0:
            // If a flag was set, do nothing else
            if (longOpts[long_index].flag != 0)
               break;
            case 'l' : loopReps = atoi(optarg);
               break;
            case 'w' : workIters = atoi(optarg);
               break;
         }
      }
   } // end-if

   // Broadcast workIters and loopReps
   MPI_Bcast(&result_flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&csv_flag,    1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&workIters,   1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&loopReps,    1, MPI_INT, 0, MPI_COMM_WORLD);

   memset(hnbuf, 0, sizeof(hnbuf));
  (void)gethostname(hnbuf, sizeof(hnbuf));

   #pragma omp master
   {
      if (csv_flag) printf("Rank, Thread, CPU ID, Core ID, NUMA Node, Host\n");
   }
   omp_set_dynamic(0);
   #pragma omp parallel private(tid, cpu_id, core_id, numa_node) shared(topology)
   {
      tid = omp_get_thread_num();
      nThreads = omp_get_num_threads();
      tscValue = generic_rdtscp(&cpu_id, &numa_node); // sets the value of cpu_id
                                                      // and numa_node
      core_id = core_from_cpu(topology, cpu_id);
      #pragma omp barrier
      if (csv_flag) {
         printf("%d, %d, %d, %d, %d, %s.\n", rank, tid, cpu_id, core_id, numa_node, hnbuf);
      } else {
         printf("Rank %d, thread %d of %d on CPU %d, core %d, NUMA node %d (%s).\n", rank, tid, nThreads, cpu_id, core_id, numa_node, hnbuf);
      }
   }

   // Serial loop
   startTime = omp_get_wtime();
   for(int j = 0; j < loopReps; j++) {
      workloop(workIters);
   }
   sTime = (omp_get_wtime() - startTime);

   // Parallel loop
   // In theory, each thread will execute workloop() a total of loopReps iterations,
   // exactly as for the case of the single-thread execution above. We will
   // get a longer execution time because of the overhead associated with
   // OMP FOR.
   #pragma omp parallel
   {
      startTime = omp_get_wtime();
      for(int j = 0; j < loopReps; j++) {
   #pragma omp for
         for(int i = 0; i < nThreads; i++) {
            workloop(workIters);
         }
      }
      pTime = (omp_get_wtime() - startTime);
   }

   // output results
   if ( rank == 0 && result_flag ) {
      printf("Results (time in seconds)\n");
      printf("Rank\tSerial\tParallel\tOverhead\tnThreads\n");
      printf("%d:\t%8.4f\t%8.4f\t%8.5f\t%8d\n", rank, sTime, pTime, (pTime-sTime), nThreads);
   }
   MPI_Barrier(MPI_COMM_WORLD);
   for (i=1;i<total_ranks;i++) {
      if ( i == rank && result_flag ) {
         printf("%d:\t%8.4f\t%8.4f\t%8.5f\t%8d\n", rank, sTime, pTime, (pTime-sTime), nThreads);
      }
      MPI_Barrier(MPI_COMM_WORLD);
   }
 
   MPI_Finalize();
   return 0;
}

// A low-memory, low cache-use loop to simulate work
// Sadly, this only works for Gnu compilers,
//     void __attribute__((optimize("O0"))) workloop(int);
//
// For Intel compilers,
#pragma optimize ("", off)
// For GCC compilers
#pragma GCC push_options
#pragma GCC optimize ("O0")
// for Cray compilers
#pragma _CRI noopt
inline
void workloop(int workIters) {
   int i;
   double a = 0.0;

   for (i = 0; i < workIters; i++)
      a += sin( (double) i);
} // workloop()
#pragma _CRI opt  // Cray
#pragma GCC pop_options // Gnu
#pragma optimize ("", on)  // Intel


// Given a physical PU (CPU) ID, return the physical core that owns it
// Physical PU indexes are guaranteed unique across a node.
int core_from_cpu(hwloc_topology_t topology, int puid)
{
  int coreid = -1;
  hwloc_obj_t parent = NULL;
  hwloc_obj_t pu_obj;

  // Find the pu obj with ID puid
  // Find the first pu object
  pu_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
  while( pu_obj != NULL ) {
    if ( puid == pu_obj->os_index) {
       parent = pu_obj->parent;
    }
    pu_obj = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PU, pu_obj);
  }

  if (parent != NULL) coreid = parent->os_index;

  return coreid;
}
