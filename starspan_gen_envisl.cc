//
// STARSpan project
// Carlos A. Rueda
// starspan_gen_envisl - generates envi output
// $Id$
//

#include "starspan.h"           
#include "traverser.h"       

#include <stdlib.h>
#include <string.h>
#include <list>
#include <assert.h>

// used to write the number of extracted pixels in header since this is
// done in two passes: the first one in which the number of
// is unknown, and a second one to update it.
#define LINES_WIDTH 8


// for selection
struct Field {
	char name[1024];
	FILE* file;
	
	Field(char* n) {
		strcpy(name, n);
	}
	
	~Field() {
		if ( file )
			fclose(file);
	}
};


/**
  * Populates the envi file
  */
class EnviSlObserver : public Observer {
public:
	// true=image, false=spectral library 
	bool envi_image;
	int typeSize;
	int numBands;
	FILE* data_file;
	FILE* header_file;
	list<Field*>* fields;
	
	OGRFeature* currentFeature;
	int numSpectra;	
	long update_offset;
	
	/**
	  * Initializes the header.
	  */
	EnviSlObserver(bool image, int bands, FILE* df, FILE* hf, list<Field*>* fields_)
	: envi_image(image), numBands(bands), 
	  data_file(df), header_file(hf), fields(fields_)
	{
		numSpectra = 0;
		
		//
		// header varies depending on type of file to create.
		//
		
		// common part
		fprintf(header_file,
			"ENVI\n" 
			"description = {Generated by starspan %s}\n", 
			STARSPAN_VERSION
		);
		
		if ( envi_image ) {
			// samples
			fprintf(header_file, "samples = 1\n");
			
			// lines:  value will be given at end of process
			fprintf(header_file, "lines = ");
			// take current file pointer to later update this:
			update_offset = ftell(header_file);
			fprintf(header_file, "%*d\n", LINES_WIDTH, 0);
			
			// bands
			fprintf(header_file, "bands = %d\n", numBands);
			
			// file type
			fprintf(header_file, "file type = ENVI Standard\n");
		}
		else {
			// spectral library.
			
			// samples:
			fprintf(header_file, "samples = %d\n", numBands);
			
			// lines:  value will be given at end of process
			fprintf(header_file, "lines = ");
			// take current file pointer to later update number of lines:
			update_offset = ftell(header_file);
			fprintf(header_file, "%*d\n", LINES_WIDTH, 0);
			
			// bands (for a spectral library is 1)
			fprintf(header_file, "bands = 1\n");
			
			// file type
			fprintf(header_file, "file type = ENVI Spectral Library\n");
		}			
		
		// common:
		
		// header offset and file type
		fprintf(header_file, "header offset = 0\n");
		
		// data type    PENDING to generalize
		fprintf(header_file, "data type = 2\n");
		
		// interleave and sensor type
		fprintf(header_file, "interleave = bip\n");
		fprintf(header_file, "sensor type = Unknown\n");
		
		// byte order (Intel)  PENDING to generalize
		fprintf(header_file, "byte order = 0\n");

		/////////////////////////////////////////////////////////
		// wavelength values:
		// NOTE: ad hoc values: unfortunately GDAL does not provide a way
		// to access the wavelength meta-attribute from source dataset.
		// A solution for this is PENDING.
		fprintf(header_file,
			"wavelength = {"
		);
		for ( int i = 0; i < numBands; i++ ) {
			if ( i > 0 ) fprintf(header_file, ",");
			fprintf(header_file, "\n  %d", i+1);
		}
		fprintf(header_file, "\n}\n");
		
		if ( !envi_image ) {    // spectral library
			// spectra names:
			fprintf(header_file,
				"spectra names = {"
			);
		}
		
		currentFeature = NULL;
	}
	
	/**
	  * nothing is done.
	  */
	~EnviSlObserver() {
	}
	

	/**
	  *
	  */
	void init(GlobalInfo& info) {
		GDALDataType bandType = info.bands[0]->GetRasterDataType();
		typeSize = GDALGetDataTypeSize(bandType) >> 3;
		
		// check ALL bands are the same type
		for ( unsigned i = 1; i < info.bands.size(); i++ ) {
			GDALDataType _bandType = info.bands[i]->GetRasterDataType();
			if ( _bandType != bandType ) {
				fprintf(stderr, 
					"Different band types\n"
					"Band %d has type %s\n"
					"Band %d has type %s\n"
						, 0+1, GDALGetDataTypeName(bandType)
						, i+1, GDALGetDataTypeName(_bandType)
				);
			}
			
		}
	}
	

	/**
	  * Used here to update currentFeature
	  */
	void intersectionFound(OGRFeature* feature) {
		currentFeature = feature;
	}

	
	/**
	  * Adds a spectrum to the output file.
	  * Each spectrum take a name with the following structure:
	  *      FID:col:row:x:y
	  */
	void addPixel(TraversalEvent& ev) { 
		int col = ev.pixel.col;
		int row = ev.pixel.row;
		void* band_values = ev.bandValues;
		
		//
		// write bands to binary file:
		//
		if ( (size_t) numBands != fwrite(band_values, typeSize, numBands, data_file) ) {
			fprintf(stderr, "Couldn't write pixel\n");
			exit(1);
		}
		
		
		if ( !envi_image ) {    // spectral library
			// add spectrum name to header
			char spectrum_name[1024];
			sprintf(spectrum_name, "%ld:%d:%d:%.3f:%.3f", 
				currentFeature->GetFID(), 
				col, row,
				ev.pixel.x, ev.pixel.y
			);
			if ( numSpectra > 0 )
				fprintf(header_file, ",");
			fprintf(header_file, "\n  %s", spectrum_name);
		}
		
		///////////////////////////////////////////////
		// add class fields if so indicated
		if ( fields ) {
			list<Field*>::const_iterator it = fields->begin();
			for ( ; it != fields->end(); it++ ) {
				Field* field = *it;
				const int i = currentFeature->GetFieldIndex(field->name);
				if ( i < 0 ) {
					fprintf(stderr, "\n\tField `%s' not found\n", field->name);
					exit(1);
				}
				OGRFieldDefn* poField = currentFeature->GetFieldDefnRef(i);
				OGRFieldType ft = poField->GetType();
				switch(ft) {
					case OFTString: {
						const char* str = currentFeature->GetFieldAsString(i);
						fprintf(field->file, "%s", str);
						break;
					}
					case OFTInteger: { 
						int val = currentFeature->GetFieldAsInteger(i);
						fprintf(field->file, "%d", val);
						break;
					}
					case OFTReal: { 
						double val = currentFeature->GetFieldAsDouble(i);
						fprintf(field->file, "%f", val);
						break;
					}
					default:
						fprintf(stderr, "addPixel: expecting: "
								"OFTString, OFTInteger, or OFTReal \n");
						exit(2);
				}
				fprintf(field->file, "\n");
			}
		}		 
		
		numSpectra++;
	}
	
	/**
	  * Finishes the header file
	  */
	void _end() {
		if ( !envi_image ) {    // spectral library
			// close spectra names section:
			fprintf(header_file, "\n}\n");
		}
		// update number of lines or samples:
		fseek(header_file, update_offset, SEEK_SET);
		fprintf(header_file, "%*d", LINES_WIDTH, numSpectra);
	}
};


/**
  * implementation
  */
int starspan_gen_envisl(
	Traverser& tr,
	const char* select_fields,
	const char* envisl_name,
	bool envi_image
) {
	Raster* rast = tr.getRaster(0);
	//Vector* vect = tr.getVector();

	// output files
	char data_filename[1024];
	if ( envi_image )
		sprintf(data_filename, "%s.img", envisl_name);
	else
		sprintf(data_filename, "%s.sld", envisl_name);
	
	char header_filename[1024];
	sprintf(header_filename, "%s.hdr", envisl_name);
	
	FILE* data_file = fopen(data_filename, "w");
	if ( !data_file ) {
		fprintf(stderr, "Couldn't create %s\n", data_filename);
		return 1;
	}

	FILE* header_file = fopen(header_filename, "w");
	if ( !header_file ) {
		fclose(data_file);
		fprintf(stderr, "Couldn't create %s\n", header_filename);
		return 1;
	}
	
	list<Field*>* fields = NULL;
	if ( select_fields ) {
		fields = new list<Field*>();
		char buff[strlen(select_fields) + 1];
		strcpy(buff, select_fields);
		for ( char* fname = strtok(buff, ","); fname; fname = strtok(NULL, ",") ) {
			Field* field = new Field(fname);
			char filename[1024];
			sprintf(filename, "%s_%s.ser", envisl_name, fname);
			field->file = fopen(filename, "w");
			if ( !field->file ) {
				fclose(header_file);
				fclose(data_file);
				fprintf(stderr, "Couldn't create %s\n", filename);
				return 1;
			}
			fields->push_back(field);
		}
	}


	
	
	int bands;
	rast->getSize(NULL, NULL, &bands);
	EnviSlObserver obs(envi_image, bands, data_file, header_file, fields);	
	tr.setObserver(&obs);
	
	tr.traverse();
	
	obs._end();
	
	fclose(header_file);
	fclose(data_file);
	if ( fields ) {
		list<Field*>::const_iterator it = fields->begin();
		for ( ; it != fields->end(); it++ )
			delete *it;
		delete fields;
	}
	fprintf(stdout, "numSpectra = %d\n", obs.numSpectra);
	fprintf(stdout, "envisl finished.\n");

	return 0;
}
		


