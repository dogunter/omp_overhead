//
// omp_overhead.c
//
// Measures OMP overhead and speedup, also reports thread affinity status.
// 1. Measure the time to execute a serial function
// 2. Measure the time to execute the same serial function on individual
//    threads. The same amount of work is performed as in the single 
//    thread case. (weak scaling)
// 3. Reports task and thread affinity information.
//      * MPI Rank ID
//      * Physical PU ID, same as hardware thread ID, same as PU# from 
//        lstopo -p output
//      * Physical core ID
//      * NUMA ID, same as socket ID
//      * Hostname
//
//  Options:
//  --results    Whether to output timing results (off by default)
//  --csv        Output in simple csv format (off by default)
//  --mpi        MPI ranks will print out their affinity info
//  --logical    Display core and PU IDs using the logical indexing
//  --nth <int>  Have the synthetic work function find the nth prime number

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

// Given a physical PU id, return the logical PU id
int lpu_from_pu(hwloc_topology_t, int);
// Get a physical core ID from a processing unit (PU) ID
int core_from_pu(hwloc_topology_t, int);
// Get a logical core index from a logial PU index
int lcore_from_pu(hwloc_topology_t , int );

// This is how we get the physical PU and node IDs.
// rdtscp:  Read Time-Stamp Counter and Processor ID IA assembly instruction
//          Introduced in Core i7 and newer.
// Read 64-bit time-stamp counter and 32-bit IA32_TSC_AUX value into EDX:EAX and ECX.
// The OS should write the core id into IA32_TSC_AUX. Linux does this, as well as 
// storing the NUMA region id
//       if (cpu_has(&cpu_data(cpu), X86_FEATURE_RDTSCP))
//          write_rdtscp_aux((node << 12) | cpu);
//
//          /*
//           * Store cpu number in limit so that it can be loaded quickly
//           * in user space in vgetcpu. (12 bits for the CPU and 8 bits for the node)
//           */ 
//
// This makes it the easist way to determine which numa region (socket) and which 
// core (actually hardware thread ID in the case of hyperthreading) a process is running on.
//
// This is a generic TSC reader if the compiler does not provide an intrinsic for it.
// (The Intel intrisic is simply __rdtscp.)
//
unsigned long generic_rdtscp(int *pu_id, int *numa_id)
{
  unsigned int a, d, c;

  __asm__ volatile("rdtscp" : "=a" (a), "=d" (d), "=c" (c));

  *numa_id = (c & 0xFFF000)>>12; // Mask off lower bits and then shift to get numa_id
  *pu_id = c & 0xFFF;            // Mask off higher bits and then read pu_id from lower 8 bits

  return ( (unsigned long)a ) | ( ((unsigned long)d ) << 32 );;
}

// A time-consuming function
long findPrimeNumber(int);

int main(int argc, char** argv) {

   int i, err, rank, total_ranks;
   int opt = 0;
   int longIndex = 0;
   int nth;
   long nthPrime;
   int tid, nThreads;

   unsigned long int tscValue;
   int pu_id, core_id, numa_id;  // Physical index values
   int lpu_id, lcore_id;         // Logically indexed pu and core id values.

   hwloc_topology_t topology;

   double startTime;
   double sTime; // Serial loop time
   double pTime; // Parallel loop time
   // MPI timing variables
   double mpi_stime, mpi_etime;

   char hnbuf[64]; // holds the hostname (node ID)

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &total_ranks);

   mpi_stime = MPI_Wtime();

   // Create the topology
   hwloc_topology_init(&topology);
   hwloc_topology_load(topology);

   // Parse options
   //  --nth <integer>> - Find the nth prime number

   if (rank == 0 ) {
      int long_index =0;
      nth = NTH;
      while ((opt = getopt_long(argc, argv,"n:", longOpts, &long_index )) != -1) {
         switch (opt) {
            case 0:
            // If a flag was set, do nothing else
            if (longOpts[long_index].flag != 0)
               break;
            case 'n' : nth = atoi(optarg);
               break;
         }
      }
   } // end-if

   // Broadcast nth
   MPI_Bcast(&result_flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&csv_flag,    1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&logical_flag,1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&mpi_flag,    1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&nth,         1, MPI_INT, 0, MPI_COMM_WORLD);

   memset(hnbuf, 0, sizeof(hnbuf));
   err = gethostname(hnbuf, sizeof(hnbuf));


   // Serial loop (every MPI rank does this)
   startTime = omp_get_wtime();
   nthPrime = findPrimeNumber(nth);
   sTime = (omp_get_wtime() - startTime);

   if (mpi_flag) {
      tscValue = generic_rdtscp(&pu_id, &numa_id); // sets the value of pu_id and numa_id
      if (logical_flag) {
         lpu_id = lpu_from_pu(topology, pu_id); 
         lcore_id = lcore_from_pu(topology, pu_id);
         printf("MPI-only: Rank %d, lPU %d, lcore %d, NUMA id %d (%s).\n", rank, lpu_id, lcore_id, numa_id, hnbuf);
      } else {
         core_id = core_from_pu(topology, pu_id);
         printf("MPI-only: Rank %d, PU %d, core %d, NUMA id %d (%s).\n", rank, pu_id, core_id, numa_id, hnbuf);
      }
   }

   #pragma omp master
   {
      if (csv_flag) printf("Rank, Thread, PU ID, Core ID, NUMA ID, Host\n");
   }
   omp_set_dynamic(0);
   #pragma omp parallel private(tid, pu_id, core_id, lpu_id, lcore_id, numa_id, nthPrime) shared(topology)
   {
      tid = omp_get_thread_num();
      nThreads = omp_get_num_threads();
      startTime = omp_get_wtime();
      #pragma omp for
      for(int i = 0; i < nThreads; i++) {
         nthPrime = findPrimeNumber( nth ); // All threads do the same amount of work
      }
      pTime = (omp_get_wtime() - startTime);
      #pragma omp barrier
      tscValue = generic_rdtscp(&pu_id, &numa_id); // sets the value of pu_id and numa_id
      if (logical_flag) {
         lpu_id = lpu_from_pu(topology, pu_id); 
         lcore_id = lcore_from_pu(topology, pu_id);
      } else {
         core_id = core_from_pu(topology, pu_id);
      }
      if (csv_flag) {
         if (logical_flag) {
            printf("%d, %d, %d, %d, %d, %s.\n", rank, tid, lpu_id, lcore_id, numa_id, hnbuf);
         } else {
            printf("%d, %d, %d, %d, %d, %s.\n", rank, tid, pu_id, core_id, numa_id, hnbuf);
         } // logical flag
      } else { // normal output
         if (logical_flag) {
            printf("OMP: Rank %d, thread %d of %d on lPU %d, lcore %d, NUMA id %d (%s).\n", rank, tid, nThreads, lpu_id, lcore_id, numa_id, hnbuf);
         } else {
            printf("OMP: Rank %d, thread %d of %d on PU %d, core %d, NUMA id %d (%s).\n", rank, tid, nThreads, pu_id, core_id, numa_id, hnbuf);
         } // logical flag
      }
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
 
   mpi_etime = MPI_Wtime();
   if (rank == 0) printf("Elapsed time: %f\n", mpi_etime - mpi_stime);

   MPI_Finalize();
   return 0;
}

// Find the nth prime number
long findPrimeNumber(int n)
{
   int count=0;
   long a = 2;
   while ( count < n )
   {
      long b = 2;
      int prime = 1;  // to check if found a prime
      while ( b * b <= a)
      {
         if ( a % b == 0 )
         {
            prime = 0;
            break;
         }
         b++;
      }
      if( prime > 0 )
      {
         count++;
      }
      a++;
   }
   return (--a);
}

// Given a physical PU index, return the logical PU index
// May have to revisit this, see if it's necessary.
// On Broadwell, for example, these are always the same
int lpu_from_pu(hwloc_topology_t topology, int puid)
{
  int lpuid = -1;
  hwloc_obj_t pu_obj;

  // Find the pu obj with physical index puid
  // Find the first pu object
  pu_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
  while( pu_obj != NULL ) {
    if ( puid == pu_obj->os_index) {
       lpuid = pu_obj->logical_index;
    }
    pu_obj = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PU, pu_obj);
  }

  return lpuid;
}

// Given a physical PU (CPU) ID, return the physical core that owns it
// Physical PU indexes are guaranteed unique across a node.
int core_from_pu(hwloc_topology_t topology, int puid)
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

// Given a physical PU index, return the logical core index that owns it
int lcore_from_pu(hwloc_topology_t topology, int puid)
{
  int lcoreid = -1;
  hwloc_obj_t parent = NULL;
  hwloc_obj_t pu_obj;

  // Find the pu obj with logical index puid
  // Find the first pu object
  pu_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
  while( pu_obj != NULL ) {
    if ( puid == pu_obj->os_index) {
       parent = pu_obj->parent;
    }
    pu_obj = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PU, pu_obj);
  }

  if (parent != NULL) lcoreid = parent->logical_index;

  return lcoreid;
}
