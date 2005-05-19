StarSpan GUI

This is an experimental implementation of a GUI for StarSpan.
This GUI provides a direct interface to the command-line executable 
`starspan' which is part of the StarSpan core system.
The actual dependency on the core system will occur at run time when
the GUI calls the executable `starspan' to perform the requested 
operations.

Prerequisites
	- The StarSpan core system must be properly installed on your system.
	  In particular, the executable `starspan' needs to be in your PATH.
	  More info at: http://starspan.casil.ucdavis.edu
	  
	- A Java Runtime Environment must be properly installed on your system.
	  In particular, the executable `java' needs to be in your PATH.
	  More info at: http://java.com

Running:
	Assuming DIR is the installation directory:
		Unix/Linux: Run DIR/bin/starspan-gui
		Windows:    Run DIR\bin\starspan-gui.bat
		            (A shortcut can be created by the installer)
	
Status:
	Experimental

-------------------------------------------------------------
$Id$	
