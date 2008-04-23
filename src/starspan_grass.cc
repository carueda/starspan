//
// StarSpan project
// Carlos A. Rueda
// starspan_grass GRASS interface -- preliminary
// $Id$
//

#include <geos/version.h>
#if GEOS_VERSION_MAJOR < 3
	#include <geos.h>
#else
	#include <geos/unload.h>
	using namespace geos::io;   // for Unload
#endif


#include "starspan.h"
#include <cassert>


///////////////////////////////////////////////////////////////////////////////
// With GRASS
#ifdef HAVE_LIBGRASS_GIS

extern "C" {
#include "grass/gis.h"
#include "grass/Vect.h"
#include "grass/glocale.h"
}


static void shift_args(int *argc, char ** argv) {
    for ( int i = 2; i < *argc; i++ ) {
        argv[i - 1] = argv[i];
    }
    (*argc)--;
}


bool use_grass(int *argc, char ** argv) {
    if ( NULL == getenv("GISBASE") ) {
        return false;
    }
    
    if ( *argc > 1 && strcmp("S", argv[1]) == 0 ) {
        shift_args(argc, argv);
        return false;
    }
    
    return true;
    
}


static char* get_module_description(const char* cmd) {
    static char module_description[5*1024];
    sprintf(module_description,
        "\n"
        "CSTARS StarSpan %s (%s %s)\n"
        "GRASS interface"
        ,
        VERSION, __DATE__, __TIME__ 
    );
    if ( cmd ) {
        strcat(module_description, " - Command: ");
        strcat(module_description, cmd);
    }
    strcat(module_description, "\n");
    return module_description;
}

static void init(const char* cmd) {
	char long_prgname[1024];
    
	struct GModule *module;
    
	sprintf(long_prgname, "%s %s (%s %s)", "starspan", VERSION, __DATE__, __TIME__);
	G_gisinit(long_prgname);
	
	module = G_define_module();
	module->description = get_module_description(cmd);
}



static Option *define_string_option(const char* key, const char* desc, int required) {
	Option *option = G_define_option() ;
	option->key        = (char*) key;
	option->type       = TYPE_STRING;
	option->required   = required;
	option->description= (char*) desc;
    return option;
}


// Calls G_parser and does general preparations:
static int call_parser(int argc, char ** argv) {
	if ( G_parser(argc, argv) ) {
		return EXIT_FAILURE;
    }
    
    globalOptions.verbose = G_verbose();
    
    return 0;
}


/** Opens a vector file if name given */
static Vector* open_vector(const char* filename) {
    if ( !filename ) {
        return 0;
    }
    
    Vector* vect;
    
    // first, try to open as a GRASS map:
    char *mapset;
    if ( (mapset = G_find_vector2(filename, "") ) != NULL ) {
        string fn = string(G_gisdbase())
                        + "/" + G_location()
                        + "/" + mapset
                        + "/vector"
                        + "/" + filename
                        + "/head";
        
        vect = Vector::open(fn.c_str());
    }
    else {
        // try to open using GDAL directly:
        vect = Vector::open(filename);
    }
    
    if ( !vect ) {
        G_fatal_error(_("Vector map <%s> not found"), filename);
    }
    return vect;
}


/** Opens a raster file if name given */
static Raster* open_raster(const char* filename) {
    if ( !filename ) {
        return 0;
    }
    
    Raster* rast;
    
    // first, try to open as a GRASS map:
    char *mapset;
    if ( (mapset = G_find_cell2(filename, "") ) != NULL ) {
        string fn = string(G_gisdbase())
                        + "/" + G_location()
                        + "/" + mapset
                        + "/cellhd"
                        + "/" + filename;
        
        rast = Raster::open(fn.c_str());
    }
    else {
        // try to open using GDAL directly:
        rast = Raster::open(filename);
    }
    
    if ( !rast ) {
        G_fatal_error(_("Raster map <%s> not found"), filename);
    }
    return rast;
}


// 
// get the layer number for the given layer name 
// if not specified or there is only a single layer, use the first layer 
//
static int process_opt_layer(Vector* vect, const char* vector_layername) {
    int vector_layernum = 0;
    if ( vector_layername && vect->getLayerCount() != 1 ) { 
        for ( unsigned i = 0; i < vect->getLayerCount(); i++ ) {    
            if ( 0 == strcmp(vector_layername, 
                     vect->getLayer(i)->GetLayerDefn()->GetName()) ) {
                vector_layernum = i;
                break;
            }
        }
    }
    else {
           vector_layernum = 0; 
    }
    return vector_layernum;
}




static vector<const char*>* process_opt_masks(Option* opt_mask_input) {
    vector<const char*>* mask_filenames = 0;    
    if ( opt_mask_input->answers ) {
        mask_filenames = new vector<const char*>();
        for ( int n = 0; opt_mask_input->answers[n] != NULL; n++ ) {
            mask_filenames->push_back(opt_mask_input->answers[n]);
        }
    }
    return mask_filenames;
}



static vector<const char*>* process_opt_fields(Option* opt_fields) {
    vector<const char*>* select_fields = NULL;
    if ( opt_fields->answer ) {
        select_fields = new vector<const char*>();
        // the special name "none" will indicate not fields at all:
        bool none = false;
        for ( int n = 0; opt_fields->answers[n] != NULL; n++ ) {
            const char* str = opt_fields->answers[n];
            none = none || 0==strcmp(str, "none");
            if ( !none ) {
                select_fields->push_back(str);
            }
        }
        if ( none ) {
            select_fields->clear();
        }
    }
    return select_fields;
}

static void process_opt_duplicate_pixel(Option *opt_duplicate_pixel) {
    if ( !opt_duplicate_pixel->answers ) {
        return;
    }
    for ( int n = 0; opt_duplicate_pixel->answers[n] != NULL; n++ ) {
        char* mode = opt_duplicate_pixel->answers[n];
        char dup_code[256];
        float dup_arg = -1;
        sscanf(mode, "%[^ ] %f", dup_code, &dup_arg);
        if ( 0 == strcmp(dup_code, "direction") ) {
            if ( dup_arg < 0 ) {
                G_fatal_error("duplicate_pixel: direction: missing angle parameter");
            }
            globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code, dup_arg));
        }
        else if ( 0 == strcmp(dup_code, "distance") ) {
            // OK.
            globalOptions.dupPixelModes.push_back(DupPixelMode(dup_code));
        }
        else {
            G_fatal_error("duplicate_pixel: unrecognized mode: %s\n", dup_code);
        }
    }
    if ( globalOptions.verbose ) {
        cout<< "duplicate_pixel modes given:" << endl;
        for (int k = 0, count = globalOptions.dupPixelModes.size(); k < count; k++ ) {
            cout<< "\t" <<globalOptions.dupPixelModes[k].toString() << endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
static int command_report(int argc, char ** argv) {
	struct Option *opt_vector_input = define_string_option("vector",  "Vector map", NO);
	struct Option *opt_raster_input = define_string_option("rasters", "Raster map(s)", NO);
    opt_raster_input->multiple = YES;

	if ( call_parser(argc, argv) ) {
		return EXIT_FAILURE;
    }
    
    if ( !opt_vector_input->answer && !opt_raster_input->answers ) {
        G_fatal_error("report: Please give at least one input map to report\n");
    }
    
	Vector* vect = open_vector(opt_vector_input->answer);
    if ( vect ) {
        starspan_report_vector(vect);
        delete vect;
    }

    for ( int n = 0; opt_raster_input->answers[n] != NULL; n++ ) {
        Raster* rast = open_raster(opt_raster_input->answers[n]);
        if ( rast ) {
            starspan_report_raster(rast);
            delete rast;
        }
    }
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
static int command_csv(int argc, char ** argv) {
	struct Option *opt_vector_input = define_string_option("vector",  "Vector map", YES);
    
	struct Option *opt_raster_input = define_string_option("rasters", "Raster map(s)", YES);
    opt_raster_input->multiple = YES;
    
	struct Option *opt_mask_input = define_string_option("masks", "Raster mask(s)", NO);
    opt_mask_input->multiple = YES;
    
    struct Option *opt_output = define_string_option("output",  "Output filename", YES);
    
    struct Option *opt_fields = define_string_option("fields",  "Desired fields", NO);
    opt_fields->multiple = YES;
    
    struct Option *opt_duplicate_pixel = define_string_option("duplicate_pixel",  "Duplicate pixel modes", NO);
    opt_duplicate_pixel->multiple = YES;
    
    struct Option *opt_layer_name = define_string_option("layer",  "Layer within vector dataset", NO);

    
	if ( call_parser(argc, argv) ) {
		return EXIT_FAILURE;
    }
    

    const char* csv_name = opt_output->answer;
    
	Vector* vect = open_vector(opt_vector_input->answer);
    if ( !vect ) {
        return EXIT_FAILURE;
    }

    vector<const char*> raster_filenames;    
    for ( int n = 0; opt_raster_input->answers[n] != NULL; n++ ) {
        raster_filenames.push_back(opt_raster_input->answers[n]);
    }

    vector<const char*>* mask_filenames = process_opt_masks(opt_mask_input);    
    if ( mask_filenames ) {
        if ( raster_filenames.size() != mask_filenames->size() ) {
            G_fatal_error(_("Different number of rasters and masks"));
        }
        
        if ( globalOptions.verbose ) {
            cout<< "raster-mask pairs given:" << endl;
            for ( size_t i = 0; i < raster_filenames.size(); i++ ) {
                cout<< "\t" <<raster_filenames[i]<< endl
                    << "\t" <<(*mask_filenames)[i]<< endl<<endl;
            }
        }
    }

    vector<const char*>* select_fields = process_opt_fields(opt_fields);
    
    process_opt_duplicate_pixel(opt_duplicate_pixel);
    
    int vector_layernum = process_opt_layer(vect, opt_layer_name->answer);

    int res;
    if ( globalOptions.dupPixelModes.size() > 0 ) {
        res = starspan_csv2(
            vect,
            raster_filenames,
            mask_filenames,
            select_fields,
            vector_layernum,
            globalOptions.dupPixelModes,
            csv_name
        );
    }
    else {    
        int res = starspan_csv(
            vect,  
            raster_filenames,
            select_fields, 
            csv_name,
            vector_layernum
        );
    }
    
    delete vect;
    
    if ( select_fields ) {
        delete select_fields;
    }

    if ( mask_filenames ) {
        delete mask_filenames;
    }
    
    return res;
}



int starspan_grass(int argc, char ** argv) {
    const char* cmd = argc == 1 ? "help" : argv[1];
    
    if ( strcmp("help", cmd) == 0 ) {
        fprintf(stdout, 
            " %s\n"
            " USAGE:  starspan <command> ...arguments...\n"
            " The following commands are implemented: \n"
            "    report ...\n"
            "    csv ...\n"
            "\n"
            "For more details about a command, type:\n"
            "    starspan <command> help\n"
            "\n"
            "To run the standard (non-GRASS) interface:\n"
            "    starspan S ...arguments...\n"
            "(or just run starspan outside of GRASS)\n"
            "\n"
            ,
            get_module_description(0)
        );
        exit(0);
    }
    else if ( strcmp("report", cmd) == 0 
    ||        strcmp("csv", cmd) == 0 
    ) {
        // OK.
    }
    else {
        G_fatal_error(_("Command <%s> not recognized/implemented"), cmd);
    }

    // command is OK.
    
	// module initialization
	Raster::init();
	Vector::init();
    
    shift_args(&argc, argv);

    init(cmd);
    char prgname[256];
    sprintf(prgname, "starspan %s", cmd);
    argv[0] = prgname;
    
                   
    int res = 0;
    if ( strcmp("report", cmd) == 0 ) {
        res = command_report(argc, argv);
    }
    else if ( strcmp("csv", cmd) == 0 ) {
        res = command_csv(argc, argv);
    }
    
	Vector::end();
	Raster::end();
	
	// more final cleanup:
	Unload::Release();	
    

    exit(res);
}


///////////////////////////////////////////////////////////////////////////////
// Not with GRASS
#else

bool use_grass(int &argc, char ** argv) {
    return false;
}

int starspan_grass(int argc, char ** argv) {
    fprintf(stderr, "ERROR: StarSpan not compiled with grass support.\n");
    return -1;
}

#endif

