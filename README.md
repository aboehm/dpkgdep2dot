# dpkgdep2dot

dpkgdep2dot is a debian package dependencies lister. The resulting dependecy
graph is in dot format (GraphViz http://www.graphviz.org/).

## Compile and Install

No further libraries are needed only GCC (G++) with C++ 2014 standard.

To compile run

	make 

Drop the executable dpkgdep2dot in your binary directory to install or to
compile and install into your system run

	make install

## Usage

*Sytnax*:
	dpkgdep2dot -l PACKAGELIST [-p PACKAGE] [-i PACKAGE]
	
	-l PACKAGELIST	- a package list, for debian the system lists are in
			  /var/lib/apt/lists/
	-p PACKAGE	- create a dependency tree only for this package
	-i PACKAGE	- ignore this package as dependency and don't show it
			  (multiple time possible)

*Example*:

	dpkgdep2dot \
		-l /var/lib/apt/lists/ftp.debian.org_debian_dists_jessie_main_binary-i386_Packages \
		-p udev \
		-i libc6 \
		-i zlib1g
		
Creates a dot representation from package udev. The base packats libc6 and
zlib1g were irgnored.

*Usage with GraphViz*:

	dpkgdep2dot \
		-l /var/lib/apt/lists/ftp.debian.org_debian_dists_jessie_main_binary-i386_Packages \
		-p udev \
		-i libc6 \
		-i zlib1g \
	|dot -Tpng -o udev-dependencies.png

Creates a PNG image of the directional dependency graph from package udev
without libc6 and zlib1g.

## License

This software is licensed under GPL v2

