//
// JTS routines
// Carlos A. Rueda
// $Id$
//

#ifndef jts_h
#define jts_h

#include <stdio.h>


/////////////////////////////////////////////////////////////
// test generation routines

/** starts the contents of the test file */
void jts_test_init(FILE* jts_test_file);

/** starts a case */
void jts_test_case_init(FILE* jts_test_file);

/** starts an arg */
void jts_test_case_arg_init(FILE* jts_test_file, char* argname);

/** ends an arg */
void jts_test_case_arg_end(FILE* jts_test_file, char* argname);

/** ends a case */
void jts_test_case_end(FILE* jts_test_file);

/** ends the test */
void jts_test_end(FILE* jts_test_file);

#endif

