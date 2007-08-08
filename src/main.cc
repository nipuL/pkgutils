//
//  pkgutils
// 
//  Copyright (c) 2000-2005 Per Liden
//  Copyright (c) 2006-2007 by CRUX team (http://crux.nu)
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

#if (__GNUC__ < 3)
#error This program requires GCC 3.x to compile.
#endif

#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>
#include <libgen.h>
#include "pkgutil.h"
#include "pkgadd.h"
#include "pkgrm.h"
#include "pkginfo.h"

using namespace std;

static pkgutil* select_utility(const string& name)
{
	if (name == "pkgadd")
		return new pkgadd;
	else if (name == "pkgrm")
		return new pkgrm;
	else if (name == "pkginfo")
		return new pkginfo;
	else
		throw runtime_error("command not supported by pkgutils");
}

int main(int argc, char** argv)
{
	string name = basename(argv[0]);

	try {
		auto_ptr<pkgutil> util(select_utility(name));

		// Handle common options
		for (int i = 1; i < argc; i++) {
			string option(argv[i]);
			if (option == "-v" || option == "--version") {
				util->print_version();
				return EXIT_SUCCESS;
			} else if (option == "-h" || option == "--help") {
				util->print_help();
				return EXIT_SUCCESS;
			}
		}

		util->run(argc, argv);
	} catch (runtime_error& e) {
		cerr << name << ": " << e.what() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
