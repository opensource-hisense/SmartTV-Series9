#ifndef ZRAM_TEST_H
#define ZRAM_TEST_H

//#include <inttypes.h>
#include <linux/types.h>

// set this to non-zero to get verbose dumping of
// test progress via printf.
extern int verbose;

// Before running the test, set this to the number of
// pages to compress in the test.  If left at zero the
// test runs until failure.
extern uint32_t num_pages;

// Runs the test.
// Before running test:
//   If requred, reset zram accelerator and bring it out
//     of reset.
//   Do any implementation-specific initialization
// Returns: 0 test passed
//          1 test failed
int zram_test(void);

#endif
