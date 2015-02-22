/*
 *  dpkgdep2dot - a debian package dependecies lister *
 *  Copyright (c) 2015, Alexander Böhm
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <stdio.h>
#include <string.h>
#include <exception>
#include <unistd.h>

#define MAX_DEP_PKG		256
#define MAX_PKG_NAME_LEN	512
#define MAX_PKG_VER_LEN		64

using namespace std;

class package_t;
class package_db_t;

typedef list<package_t*> package_list_t;
typedef list<package_t*>::iterator package_list_it_t;

typedef map<string, package_t*> package_map_t;
typedef map<string, package_t*>::iterator package_map_it_t;

class package_t {
public:
	package_t();
	package_t(string name);
	~package_t();
	string node_name();
	int parse_depends (char* line_buf, const size_t line_len);
	void add_dependency (string name);

	string			name;
	string			version;

	map<string, package_t*>	depended;

	int			marked;
};

class package_db_t {
public:
	package_db_t();
	~package_db_t();
	static package_db_t* get();
	package_t* get_or_create(string name);
	package_t* get_by_name(string name);
	int read_pkg_list (const string filename);
	void to_graphviz (map<string, bool> ignore);
	void to_graphviz (string name, map<string, bool> ignore);

	map<string, package_t*>		db;
	static package_db_t*		instance;
};

package_db_t* package_db_t::instance = NULL;

package_t::package_t() {
	this->name = "";
	this->version = "";
	this->marked = 0;
	this->depended = map<string, package_t*>();
}

package_t::package_t(string name) {
	this->name = name;
	this->version = "";
	this->marked = 0;
	this->depended = map<string, package_t*>();
}

package_t::~package_t() {
}

string package_t::node_name() {
	string r = "__";
	r.append(name);
	
	for (size_t i=0 ; i<r.length() != 0 ; i++) {
		switch (r[i]) {
		case '-':
		case '+':
		case '.':
			r[i] = '_';
			break;
		}
	}

	return r;
}

int package_t::parse_depends (char* line_buf, const size_t line_len) {
	size_t idx = 9;
	char pkg_name[MAX_PKG_NAME_LEN];
	char v[4096];
	char *p, *p2;

	idx = 9;
	p = &line_buf[idx];
	p2 = NULL;

	while (p && idx <= line_len) {
		p2 = strsep(&p, ",");
		if (p == NULL)
			idx = line_len;
	
		sscanf(p2, "%s", v);
		this->add_dependency(v);
	}

	return 0;
}

void package_t::add_dependency (string name) {
	package_db_t* db = package_db_t::get();
	depended[name] = db->get_or_create(name);
}

package_db_t::package_db_t() {
	this->db = map<string, package_t*>();

	if (!package_db_t::instance)
		package_db_t::instance = this;
}

package_db_t::~package_db_t() {
}

package_db_t* package_db_t::get() {
	if (!package_db_t::instance)
		package_db_t::instance = new package_db_t();

	return package_db_t::instance;
}

package_t* package_db_t::get_or_create(string name) {
	package_map_it_t i = this->db.find(name);

	if (i == this->db.end()) {
		this->db[name] = new package_t(name);
	} 

	return this->db[name];
}

package_t* package_db_t::get_by_name(string name) {
	try {
		return this->db.find(name)->second;
	} catch (out_of_range e) {
		return NULL;
	}
}

int package_db_t::read_pkg_list (const string filename) {
	FILE* f = fopen(filename.c_str(), "r");
	char* line_buf;
	size_t line_len, idx;
	package_t* pkg = NULL;
	char key[512];
	char value[4096];

	if (f == NULL)
		return 1;

	while (1) {
		line_buf = NULL;
		line_len = 0;
	
		int line_rlen = getdelim(&line_buf, &line_len, 0x0a, f);
		if (line_buf == NULL || line_rlen <= 0)
			break;

		if (sscanf(line_buf, "%s %s", key, value) > 0) {
			if (!strcmp(key, "Package:")) {
				if (pkg == NULL)
					pkg = this->get_or_create(value);
				
			} else if (!strcmp(key, "Version:")) {
				pkg->version = value;
			} else if (!strcmp(key, "Depends:")) {
				pkg->parse_depends (line_buf, line_len);
			} else {
			}

		} else {
			pkg = NULL;
		}

		if (line_buf != NULL)
			free(line_buf);
	}
	
	if (line_buf != NULL)
		free(line_buf);

	fclose(f);
}

void package_db_t::to_graphviz (string name, map<string, bool> ignore) {
	package_t* pkg = this->get_by_name(name);
	package_list_t pkgs;
	map<string, bool> marked;
	package_list_it_t pkg_it;

	cout << "digraph pkgdep {\n";

	if (pkg != NULL) {
		pkgs.push_back(this->get_by_name(name));

		while (!pkgs.empty()) {
			pkg = pkgs.back();
			pkgs.pop_back();

			try {
				ignore.at(pkg->name);
				continue;
			} catch (out_of_range e) {
			}

			try {
				marked.at(pkg->name);
			} catch (out_of_range e) {
				for (package_map_it_t i=pkg->depended.begin() ; i != pkg->depended.end() ; i++) {
					pkgs.push_front(i->second);
				}
				marked[pkg->name] = true;
			}
		}

		for (map<string, bool>::iterator i=marked.begin() ; i!=marked.end() ; i++) {
			package_t *p = this->get_by_name(i->first);
			cout << "\t" << p->node_name() << " [label=\"" << p->name << "\"]" << endl;
		}

		for (map<string, bool>::iterator i=marked.begin() ; i!=marked.end() ; i++) {
			package_t *p = this->get_by_name(i->first);

			for (package_map_it_t j=p->depended.begin() ; j != p->depended.end() ; j++) {
				try {
					ignore.at(j->second->name);
					continue;
				} catch (out_of_range e) {
					cout << "\t" << p->node_name() << " -> " << j->second->node_name() << endl;
				}
			}
		}

	}
	
	cout << "}\n";
}

void package_db_t::to_graphviz (map<string, bool> ignore) {
	cout << "digraph pkgdep {\n";

	for (package_map_it_t i=this->db.begin() ; i != this->db.end() ; i++) {
		cout << "\t" << i->second->node_name() << " [label=\"" << i->second->name << "\"]" << endl;
	}
	
	for (package_map_it_t i=this->db.begin() ; i != this->db.end() ; i++) {
		package_t *p = i->second;

		for (package_map_it_t j=p->depended.begin() ; j != p->depended.end() ; j++) {
			try {
				ignore.at(j->second->name);
				continue;
			} catch (out_of_range e) {
				cout << "\t" << p->node_name() << " -> " << j->second->node_name() << endl;
			}
		}
	}

	cout << "}\n";
}

void print_help_and_exit(char* arg0) {
	fprintf(stdout, "debian package dependencies to dot converter - Alexander Böhm (2015)\n");
	fprintf(stdout, "syntax: %s -l <PACKAGELIST> [-p PACKAGE] [-i PACKAGE]\n", arg0);
	fprintf(stdout, "\t -l PACKAGELIST - a package list maybe from /var/lib/apt/lists/\n");
	fprintf(stdout, "\t -p PACKAGE     - list all depended packages for PACKAGE\n");
	fprintf(stdout, "\t -i PACKAGE     - ignore this package\n");
	fprintf(stdout, "\n");
	exit(0);
}

string			opt_from_package = "";
string			opt_list_file = "";
map<string, bool>	opt_ignore_pkgs = map<string, bool>();

void parse_options(int argc, char* argv[]) {
	int opt;

	while ((opt=getopt(argc, argv, "l:p:hi:")) != -1) {
		switch (opt) {
		case 'l':
			opt_list_file = string(optarg);
			break;
		case 'p':
			opt_from_package = string(optarg);
			break;
		case 'i':
			opt_ignore_pkgs[string(optarg)] = true;
			break;
		case 'h':
			print_help_and_exit(argv[0]);
			break;
		default:
			fprintf(stderr, "\n");
			print_help_and_exit(argv[0]);
		}
	}

	if (opt_list_file.empty()) {
		fprintf(stderr, "no package list specified! you need one\n\n");
		print_help_and_exit(argv[0]);
	}
}

int main (int argc, char* argv[]) {
	package_db_t db;

	parse_options(argc, argv);
	db.read_pkg_list(opt_list_file);

	if (!opt_from_package.empty())
		db.to_graphviz(opt_from_package, opt_ignore_pkgs);
	else 
		db.to_graphviz(opt_ignore_pkgs);

	return 0;
}

