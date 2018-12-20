static int verbose_flag;
static int result_flag;
static int csv_flag;
static int logical_flag;
static int mpi_flag;

// option parsing structures
static const struct option longOpts[] = {
   { "verbose", no_argument, &verbose_flag, 0},
   { "results", no_argument, &result_flag, 1},
   { "logical", no_argument, &logical_flag, 1},
   { "mpi", no_argument, &mpi_flag, 1},
   { "csv", no_argument, &csv_flag, 1},
   { "nth", required_argument, NULL, 'n' }
};

// Set some defaults
const int NTH = 100000;
