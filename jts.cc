//
// STARSpan project
// JTS routines
// Carlos A. Rueda
// $Id$
// See jts.h for public documentation
//

#include "jts.h"           

void jts_test_init(FILE* jts_test_file) {
	if ( jts_test_file ) {
		fprintf(jts_test_file, "<run>\n");
		fprintf(jts_test_file, "  <desc>testset created by starspan</desc>\n");
		fprintf(jts_test_file, "  <precisionModel type=\"FLOATING\" />\n");
	}
}
void jts_test_case_init(FILE* jts_test_file) {
	if ( jts_test_file ) {
		fprintf(jts_test_file, "  <case>\n");
		fprintf(jts_test_file, "    <desc>geometry contains points</desc>\n");
	}
}
void jts_test_case_arg_init(FILE* jts_test_file, char* argname) {
	if ( jts_test_file ) {
		fprintf(jts_test_file, "    <%s>\n", argname);
	}
}
void jts_test_case_arg_end(FILE* jts_test_file, char* argname) {
	if ( jts_test_file ) {
		fprintf(jts_test_file, "    </%s>\n", argname);
	}
}
void jts_test_case_end(FILE* jts_test_file) {
	if ( jts_test_file ) {
		fprintf(jts_test_file, "    <test>\n");
		fprintf(jts_test_file, "      <op name=\"contains\" arg1=\"a\" arg2=\"b\">true</op>\n");
		fprintf(jts_test_file, "    </test>\n");
		fprintf(jts_test_file, "  </case>\n");
	}
}
void jts_test_end(FILE* jts_test_file) {
	if ( jts_test_file ) {
		fprintf(jts_test_file, "</run>\n");
	}
}

