#
# Stript to make STARSPAN source distribution
# $Id$
#
if [ -z $1 ]; then
	echo "USAGE:  mksrcdist.sh <VERSION> [-dotag]"
	echo "  -dotag:  make cvs -q tag release-<VERSION> before"
	exit 1
fi
VERSION=$1
dotag=$2
if [ "$VERSION" != `cat VERSION` ]; then
	echo "VERSION parameter must be equal to VERSION file contents"
	exit 2
fi
tag=`echo release-$VERSION | tr -t . _`
cvsroot=":pserver:anonymous@cvs.casil.ucdavis.edu:/cvsroot/starspan"

(test "$dotag" != "-dotag" || cvs -q tag $tag) &&\
mkdir -p DISTDIR &&\
cd DISTDIR &&\
cvs -d$cvsroot -q export -r $tag -d starspan-$VERSION starspan &&\
tar cf starspan-$VERSION.tar starspan-$VERSION &&\
gzip -9 starspan-$VERSION.tar &&\
rm -rf starspan-$VERSION &&\
ls -l &&\
echo "Done."

