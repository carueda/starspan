/**
 * @opt all
 * @opt types
 * @hidden
 */
class UMLOptions {}

/** @hidden */  class OGRLayer {}
/** @hidden */  class GDALDataset {}
/** @hidden */  class OGREnvelope {}
/** @hidden */  class GDALDataType {}
/** @hidden */  class Signature {}
/** @hidden */  class DBFHandle {}
/** @hidden */  class FILE {}


/** 
  * @stereotype implicit 
  * @navassoc - <creates> - Raster 
  * @navassoc - <creates> - Vector 
  * @navassoc - <creates> - Traverser 
  * @navassoc - <creates> - DBObserver 
  * @navassoc - <creates> - EnviSlObserver 
  */
class Starspan {}

class Raster {
	public GDALDataset getDataset();
}

class Vector {
	public OGRLayer getLayer(int layer_num);
	public void report(FILE file);
}

/**
  * @assoc - - - Observer 
  * @depend - <uses> - Raster 
  * @depend - <uses> - Vector 
  */
class Traverser {
	public void setObserver(Observer aObserver);
	public Observer getObserver();
}

interface Observer {
	public void intersection(int fid, OGREnvelope e);
	public void addSignature(double x, double y, Signature s, GDALDataType t, int ts);
}


class DBObserver implements Observer {
	public DBFHandle file;
}

class EnviSlObserver implements Observer {
	public FILE data_file;
	public FILE header_file;
}

