//
// Based on SpearfishSample.java (geotools)
// Carlos Rueda
// $Id$
//

package starspan.tools;

import java.awt.Color;
import java.awt.Font;
import java.net.URL;

import javax.swing.JFrame;
import javax.swing.WindowConstants;

import java.io.File;

import org.geotools.data.FeatureSource;
import org.geotools.data.arcgrid.ArcGridDataSource;
import org.geotools.data.shapefile.ShapefileDataStore;
import org.geotools.feature.FeatureCollection;
import org.geotools.gui.swing.StyledMapPane;
import org.geotools.map.DefaultMapContext;
import org.geotools.map.MapContext;
import org.geotools.renderer.j2d.RenderedMapScale;
import org.geotools.styling.ColorMap;
import org.geotools.styling.Graphic;
import org.geotools.styling.LineSymbolizer;
import org.geotools.styling.Mark;
import org.geotools.styling.PointSymbolizer;
import org.geotools.styling.PolygonSymbolizer;
import org.geotools.styling.RasterSymbolizer;
import org.geotools.styling.Rule;
import org.geotools.styling.Style;
import org.geotools.styling.StyleBuilder;
import org.geotools.styling.Symbolizer;
import org.geotools.styling.TextSymbolizer;

/**
 * A simple viewer
 */
public class ShapeViewer {

    public static void main(String[] args) throws Exception {
        // Prepare feature source
		
        String basePath = "/home/carueda/cstars/GeoTools/spearfishdemo/data/";
        
        // ... roads
        ShapefileDataStore dsRoads = new ShapefileDataStore( new File( basePath + "Area_gen" ).toURL() );
        FeatureSource fsRoads = dsRoads.getFeatureSource("Area_gen");
        
        // ... streams
        ShapefileDataStore dsStreams = new ShapefileDataStore( new File( basePath + "E_parrot" ).toURL() );
        FeatureSource fsStreams = dsStreams.getFeatureSource("E_parrot");
        

        // Prepare styles
        StyleBuilder sb = new StyleBuilder();
        
        // ... streams style
        LineSymbolizer lsStream = sb.createLineSymbolizer(Color.BLUE, 3);
        Style streamsStyle = sb.createStyle(lsStream);
        
        // ... roads style
        LineSymbolizer ls1 = sb.createLineSymbolizer(Color.YELLOW, 1);
        LineSymbolizer ls2 = sb.createLineSymbolizer(Color.BLACK, 5);
        Style roadsStyle = sb.createStyle();
        roadsStyle.addFeatureTypeStyle(sb.createFeatureTypeStyle(null, sb.createRule(ls2)));
        roadsStyle.addFeatureTypeStyle(sb.createFeatureTypeStyle(null, sb.createRule(ls1)));
        
        // Build the map
        MapContext map = new DefaultMapContext();
        map.addLayer(fsStreams, streamsStyle);
        map.addLayer(fsRoads, roadsStyle);

        // Show the map
        StyledMapPane mapPane = new StyledMapPane();
        mapPane.setMapContext(map);
        mapPane.getRenderer().addLayer(new RenderedMapScale());
        JFrame frame = new JFrame();
        frame.setTitle("Delta");
        frame.setContentPane(mapPane.createScrollPane());
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        frame.setSize(640, 480);
        frame.show();
    }
}
