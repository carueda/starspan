// a test for Progress
#include "Progress.h"

int main() {
	long size = 500;
	
	Progress* p;

	cout<< "size=" << size<< "  %incr=" << 10<< ": ";	
	p = new Progress(size, 10);
	for ( int i = 0; i < size; i++ )
		p->update();
	p->end();
	delete p;
	cout << endl;
	
	cout<< "chunks of size=" << size<< ": ";
	p = new	Progress(size);
	for ( int i = 0; i < size*7 - 123; i++ )
		p->update();
	p->end();
	delete p;
	cout << endl;
	
	return 0;
}
