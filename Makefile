.PHONY: clean cleanall intel cray gnu pgi

# What system are we on?
#ifeq ($(PELANL_PRGENV),true)
#HWLOCROOT=/users/dog/hwloc-knl
#else
#HWLOCROOT=/users/dog/software/$(SYSNAME)
#endif
export HWLOCROOT

intel:
	$(MAKE) clean
	$(MAKE) -f make.$@
	-mv omp_overhead omp_overhead.INTEL

pgi:
	$(MAKE) clean
	$(MAKE) -f make.$@
	-mv omp_overhead omp_overhead.PGI

cray:
	$(MAKE) clean
	$(MAKE) -f make.$@
	-mv omp_overhead omp_overhead.CRAY

gnu:
	$(MAKE) clean
	$(MAKE) -f make.$@
	-mv omp_overhead omp_overhead.GNU

arm:
	$(MAKE) clean
	$(MAKE) -f make.$@
	-mv omp_overhead omp_overhead.ARM

clean:
	-rm -f *.o *.lst

cleanall: clean
	$(MAKE) clean
	-rm -f *.INTEL *.CRAY *.GNU *.PGI *.ARM

