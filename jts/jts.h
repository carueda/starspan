//
// JTS routines
// Carlos A. Rueda
// $Id$
//

#ifndef jts_h
#define jts_h

#include <stdio.h>


/**
  * Test generator.
  */
class JTS_TestGenerator {
public:
	/**
	  * creates a JTS test suit.
	  */
	JTS_TestGenerator(const char* filename);
	
	/**
	  * calls end();
	  */
	~JTS_TestGenerator();
	
	const char* getFilename(void) { return filename; }
	FILE* getFile(void) { return file; }
	
	/**
	  * opens a case.
	  */
	void case_init(const char* description);

	/**
	  * opens an argument spec
	  */
	void case_arg_init(const char* argname);

	/**
	  * closes an argument spec
	  */
	void case_arg_end(const char* argname);

	/**
	  * opens a case.
	  */
	void case_end(void);
	
	/**
	 * completes and closes the file containing the test suit.
	 * If no cases were added, the file is removed.
	 */
	void end();
	
private:
	const char* filename;
	FILE* file;
	int no_cases;
};



#endif

