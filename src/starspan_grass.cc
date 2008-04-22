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
	struct Option *opt_vector_input, *opt_raster_input;
    
    struct Map_info In, Out;
    char *mapset;
    
	prog_name = argv[0];
	sprintf(long_prgname, "%s %s (%s %s)", prog_name, prog_version, __DATE__, __TIME__);
	G_gisinit(long_prgname);
	
	module = G_define_module();
	sprintf(module_description, 
		"%s - CSTARS StarSpan\n"
		"Version %s (%s %s)\n"
		"\n"
		"GRASS GIS command-line interface -- NOT YET IMPLEMENTED\n"
		"\n"
		prog_name, prog_version,
        __DATE__, __TIME__
    );
	module->description = module_description;
    
	// options:					        
	opt_vector_input = G_define_option() ;
	opt_vector_input->key        = "vector";
	opt_vector_input->type       = TYPE_STRING;
	opt_vector_input->required   = YES;
	opt_vector_input->description= "Name of the vector map";

	opt_raster_input = G_define_option() ;
	opt_raster_input->key        = "raster";
	opt_raster_input->type       = TYPE_STRING;
	opt_raster_input->required   = YES;
	opt_raster_input->description= "Name of the raster map";

	if ( G_parser(argc, argv) ) {
		exit(EXIT_FAILURE);
    }
    
    
    if ( (mapset = G_find_vector2(opt_vector_input->answer, "") ) == NULL ) {
        G_fatal_error(_("Vector map <%s> not found"), opt_vector_input->answer);
    }
    
    Vect_set_open_level(2);
 	
    if (1 > Vect_open_old(&In, opt_vector_input->answer, mapset)) {
        G_fatal_error(_("Unable to open vector map <%s>"), opt_vector_input->answer);
    }
    
	string vector_input_name = opt_vector_input->answer;
	string raster_input_name = opt_raster_input->answer;
    
    
    cout<< " vector = " <<vector_input_name<< endl;
    cout<< " raster = " <<raster_input_name<< endl;
    
    
    cout<< "returning --  GRASS interface not implemented yet." <<endl;
    
    
    Vect_close(&In);
    //Vect_close(&Out);
 	
    exit(EXIT_SUCCESS);
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Not with GRASS
#else
int starspan_grass(int argc, char ** argv) {
    fprintf(stderr, "ERROR: StarSpan not compiled with grass support.\n");
    return -1;
}

#endif

