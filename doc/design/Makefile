
all: classdiagram.eps classdiagram.png \
	envisl-sd.eps envisl-sd.png

# class diagrams

classdiagram.png: classdiagram.eps
	convert classdiagram.eps classdiagram.png
	
classdiagram.eps: classdiagram.dot
	dot -Tps -oclassdiagram.eps classdiagram.dot
	
classdiagram.dot: classdiagram.java
	javadoc -docletpath ~/prg/UMLGraph-2.8/UmlGraph.jar\
	        -doclet UmlGraph \
			-output classdiagram.dot \
			-private classdiagram.java

# sequence diagrams

envisl-sd.png: envisl-sd.eps
	convert envisl-sd.eps envisl-sd.png

envisl-sd.eps: envisl-sd.pic
	pic2plot -Tps envisl-sd.pic > envisl-sd.eps
	


