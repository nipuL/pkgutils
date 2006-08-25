//
//  pkgutils
// 
//  Copyright (c) 2000-2005 Per Liden
// 
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
//  USA.
//

#ifndef PKGUTIL_H
#define PKGUTIL_H

#include <string>
#include <set>
#include <map>
#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>

#define PKG_EXT         ".pkg.tar.gz"
#define PKG_DIR         "var/lib/pkg"
#define PKG_DB          "var/lib/pkg/db"
#define PKG_REJECTED    "var/lib/pkg/rejected"
#define VERSION_DELIM   '#'
#define LDCONFIG        "/sbin/ldconfig"
#define LDCONFIG_CONF   "/etc/ld.so.conf"

using namespace std;

class pkgutil {
public:
	struct pkginfo_t {
		string version;
		set<string> files;
	};

	typedef map<string, pkginfo_t> packages_t;

	explicit pkgutil(const string& name);
	virtual ~pkgutil() {}
	virtual void run(int argc, char** argv) = 0;
	virtual void print_help() const = 0;
	void print_version() const;

protected:
	// Database
	void db_open(const string& path);
	void db_commit();
	void db_add_pkg(const string& name, const pkginfo_t& info);
	bool db_find_pkg(const string& name);
	void db_rm_pkg(const string& name);
	void db_rm_pkg(const string& name, const set<string>& keep_list);
	void db_rm_files(set<string> files, const set<string>& keep_list);
	set<string> db_find_conflicts(const string& name, const pkginfo_t& info);

	// Tar.gz
	pair<string, pkginfo_t> pkg_open(const string& filename) const;
	void pkg_install(const string& filename, const set<string>& keep_list, const set<string>& non_install_files) const;
	void pkg_footprint(string& filename) const;
	void ldconfig() const;

	string utilname;
	packages_t packages;
	string root;
};

class db_lock {
public:
	db_lock(const string& root, bool exclusive);
	~db_lock();
private:
	DIR* dir;
};

class runtime_error_with_errno : public runtime_error {
public:
	explicit runtime_error_with_errno(const string& msg) throw()
		: runtime_error(msg + string(": ") + strerror(errno)) {}
};

// Utility functions
void assert_argument(char** argv, int argc, int index);
string itos(unsigned int value);
string mtos(mode_t mode);
int unistd_gzopen(char* pathname, int flags, mode_t mode);
string trim_filename(const string& filename);
bool file_exists(const string& filename);
bool file_empty(const string& filename);
bool file_equal(const string& file1, const string& file2);
bool permissions_equal(const string& file1, const string& file2);
void file_remove(const string& basedir, const string& filename);

#endif /* PKGUTIL_H */
