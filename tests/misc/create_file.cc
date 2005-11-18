//
//    g++ -D_FILE_OFFSET_BITS=64 -Wall create_file.cc -o create_file
//
//  This program creates a file with a given size
//  See usage message
//

#include <cstdio>
#include <iostream>
using namespace std;

int main(int argc, char** argv) {
	if ( argc != 3 ) {
		cerr<< " USAGE:  create_file  filename  size_incr\n";
		cerr<< "     filename will be 2147483647 + size_incr bytes size\n";
		return 1;
	}
	const char* filename = argv[1];
	FILE* file = fopen(filename, "w");
	if ( !file ) {
		cerr<< " could not create " << filename << endl;
		return 1;
	}
	off_t incr = atol(argv[2]);
	off_t size = 2147483647 + incr;
	cout<< "  seeking one byte before size: " << size-1 << endl;
	fseeko(file, size-1, SEEK_SET);
	cout<< "  writing a byte "<< endl;
	fputc('\0', file);

	cout<< "  closing.\n";
	fclose(file);
	cout<< "  done.\n";
	return 0;
}
