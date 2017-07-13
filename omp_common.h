static int verbose_flag;
static int result_flag;
static int csv_flag;

// option parsing structures
static const struct option longOpts[] = {
   { "verbose", no_argument, &verbose_flag, 0},
   { "results", no_argument, &result_flag, 1},
   { "csv", no_argument, &csv_flag, 1},
   { "work-iters", required_argument, NULL, 'w' },
   { "loop-reps", required_argument, NULL, 'l' }
};

// Set some defaults
const int WORKITERS = 100000;
const int LOOPREPS = 1000;
