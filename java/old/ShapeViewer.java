/*
	$Id$
*/

import java.awt.*;
import java.awt.image.*;

import javax.swing.*;

public class ShapeViewer extends JPanel  {
	Polygon polygon;
	public ShapeViewer() {
		super(new GridLayout(1,1));
		polygon = new Polygon();
	}
	
	public void addPoint(int x, int y) {
		polygon.addPoint(x, y);
	}
		
	
	public void paint(Graphics g) {
		g.drawPolygon(polygon);
	}
	
	public static void main(String[] args) {
		int width = 200;
		int height = 300;
		
		ShapeViewer sv = new ShapeViewer();
		sv.addPoint(20, 100);
		sv.addPoint(70, 190);
		sv.addPoint(40, 70);
		sv.addPoint(50, 100);
		
		JFrame f = new JFrame("test");
		sv.setPreferredSize(new Dimension(width, height));
		f.getContentPane().add(sv);
		f.pack();
		f.show();
	}
}
