/*
	$Id$
*/

import java.awt.*;
import java.awt.image.*;

import javax.swing.*;

public class Rasterizer  {
	public static BufferedImage rasterizePolygon(
				int[] xPoints, int[] yPoints, int nPoints,
				int width, int height) {
		
		BufferedImage img = new BufferedImage(
			width, height, 
			BufferedImage.TYPE_BYTE_BINARY
		);
		Graphics2D g2d = img.createGraphics();
		g2d.fillPolygon(xPoints, yPoints, nPoints);
		return img;
	}
	
	public static void main(String[] args) {
		int[] xPoints = { 20,   70, 40, 50 };		
		int[] yPoints = { 100, 190, 70,  100 };
		int nPoints = xPoints.length;
		int width = 200;
		int height = 300;
		
		final BufferedImage img = rasterizePolygon(xPoints, yPoints, nPoints, width, height);
		JFrame f = new JFrame("test");
		JPanel p = new JPanel() {
			public void paint(Graphics g) {
				System.out.println("CCC");
				g.drawImage(img, 0, 0, null);
			}
		};
		p.setPreferredSize(new Dimension(width, height));
		f.getContentPane().add(p);
		f.pack();
		f.show();
	}
}
