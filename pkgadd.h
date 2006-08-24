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

#ifndef PKGADD_H
#define PKGADD_H

#include "pkgutil.h"
#include <vector>
#include <set>

#define PKGADD_CONF             "/etc/pkgadd.conf"
#define PKGADD_CONF_MAXLINE     1024

enum rule_event_t {
	UPGRADE
};

struct rule_t {
	rule_event_t event;
	string pattern;
	bool action;
};

class pkgadd : public pkgutil {
public:
	pkgadd() : pkgutil("pkgadd") {}
	virtual void run(int argc, char** argv);
	virtual void print_help() const;

private:
	vector<rule_t> read_config() const;
	set<string> make_keep_list(const set<string>& files, const vector<rule_t>& rules) const;
	void find_rules(const vector<rule_t>& rules, rule_event_t event, vector<rule_t>& found) const;
	bool rule_applies_to_file(const rule_t& rule, const string& file) const;
};

#endif /* PKGADD_H */
