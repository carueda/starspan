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
	JTS_TestGenerator(const char* filename);
	~JTS_TestGenerator();
	
	const char* getFilename(void) { return filename; }
	FILE* getFile(void) { return file; }
	
	void case_init(const char* description);
	void case_arg_init(const char* argname);
	void case_arg_end(const char* argname);
	void case_end(void);
	void end();
	
private:
	const char* filename;
	FILE* file;
};



#endif

