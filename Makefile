CXXFLAGS = -std=c++14

all: dpkgdep2dot

dpkgdep2dot: dpkgdep2dot.cpp

install: dpkgdep2dot
	install -m 755 dpkgdep2dot /usr/local/bin/dpkgdep2dot

uninstall:
	rm -f /usr/local/bin/dpkgdep2dot

clean:
	rm -f dpkgdep2dot
