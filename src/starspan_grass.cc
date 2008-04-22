//
// StarSpan project
// Carlos A. Rueda
// starspan_grass GRASS front-end program -- very preliminary
// $Id$
//

#include "starspan.h"


///////////////////////////////////////////////////////////////////////////////
// With GRASS
#ifdef HAVE_LIBGRASS_GIS

extern "C" {
#include "grass/gis.h"
}

static const char* prog_version = VERSION;
static char* prog_name;   // argv[0]



int starspan_grass(int argc, char ** argv) {
    
	char long_prgname[1024];
	char module_description[5*1024];
    
	struct GModule *module;
	struct Option *opt_input, *opt_output;
    
	prog_name = argv[0];
	sprintf(long_prgname, "%s %s (%s %s)", prog_name, prog_version, __DATE__, __TIME__);
	G_gisinit(long_prgname);
	
	module = G_define_module();
	sprintf(module_description, 
		"%s - CSTARS Daymet Interpolator\n"
		"Version %s (%s %s)\n"
		"\n"
		"Reads a sites file and generates a raster file with interpolated values\n"
		"for the active region, using an implementation of the Daymet technique.\n"
		"By default, interpolation of a pixel is computed at its center, but other\n"
		"location within the pixel can be specified.\n"
		"\n"
		"Use elevation=name to specify an elevation raster. In this case,\n"
		"unless the -n option is given, correction by elevation will be applied\n"
		"on each cell in generated raster.\n"
		"\n"
		"If a lapse rate is given, sites values are taken as follows\n"
		"     new_value = elevation * lapseRate / 1000 + original_value\n"
		"The rest of the process is done on the new values (no back-correction).\n"
		"\n"
		"Specify -cve and optionally -K, to generate a crossvalidation error report.\n"
		"Note that the interpolation step is done only if output=name is given.",
		prog_name, prog_version,
        __DATE__, __TIME__
    );
	module->description = module_description;
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Not with GRASS
#else
int starspan_grass(int argc, char ** argv) {
    fprintf(stderr, "ERORR: StarSpan not compiled with grass support.\n");
    return -1;
}

#endif

