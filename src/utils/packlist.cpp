/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of Sfera.                                           *
 *                                                                         *
 *   Sfera is free software; you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Sfera is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

#include "utils/packlist.h"

PackList::PackList() {
	// Build the list of packs
	string dir("gamedata/packs");
	SFERA_LOG("Looking for packs in: " << dir);
	try {
		path dirPath(dir);

		if (!exists(dirPath))
			throw runtime_error(dir + " directory doesn't exist");
		if (!is_directory(dirPath))
			throw runtime_error(dir + " is not a directory");

        vector<path> packDirs;
        copy(directory_iterator(dirPath), directory_iterator(), back_inserter(packDirs));
        sort(packDirs.begin(), packDirs.end());

		for (vector<path>::const_iterator it(packDirs.begin()); it != packDirs.end(); ++it) {
			string packName = it->filename().generic_string();
			SFERA_LOG("  " << packName);

			// Check if it is the definition of a pack
			if (is_directory(dir + "/" + packName)) {
				SFERA_LOG("    ACCEPTED");
				names.push_back(packName);
			} else {
				SFERA_LOG("    REJECTED");
			}
		}
	} catch (const filesystem_error &ex) {
		SFERA_LOG("Error reading the packs in: " << dir << endl << "Error: "<< ex.what());
	}
}
