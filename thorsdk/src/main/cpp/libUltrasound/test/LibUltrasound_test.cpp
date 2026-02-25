//
// This file is a placeholder and is specifically named so that the runtest.py
// script will work correctly.  All of the libUltrasound unittest source files
// are being built into a single executable.  runtest.py looks for all native
// source files that have the following filename matches:
// 
//  - test_*
//  - *_test.[cc|cpp]
//  - *_unittest.[cc}cpp]
//
// By making this the only file that adheres to the above naming, running the
// runtest.py against this directory will result in all tests executing in the
// library.
// 
// See development/testrunner/test_defs/native_test.py and 
// development/testrunner/runtest.py for more information.
//
// The alternative approach is to create separate test executables for each
// source file.
//

#include <stdio.h>


