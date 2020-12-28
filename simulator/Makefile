# Makefile wrapper for waf

all:
	cd openflow && make
	make configure
	./waf build

# free free to change this part to suit your requirements
configure:
	CXXFLAGS="-Wall" ./waf configure --with-openflow=openflow -d optimized 

configure_perf:
	CXXFLAGS="-Wall" ./waf configure --with-openflow=openflow --enable-static

build:
	./waf build

install:
	./waf install

clean:
	./waf clean

distclean:
	./waf distclean

opreport:
	opreport --exclude-dependent --demangle=smart --symbols --threshold=1 --exclude-symbols=/usr/bin/python2.7 > opreport.out

