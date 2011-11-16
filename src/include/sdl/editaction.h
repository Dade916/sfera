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

#ifndef _SFERA_EDITACTION_H
#define	_SFERA_EDITACTION_H

#include <set>

#include "sfera.h"

enum EditAction {
	CAMERA_EDIT, // Use this for any Camera parameter editing
	GEOMETRY_EDIT, // Use this for any objects editing
	MATERIALS_EDIT // Use this for any Material related editing
};

class EditActionList {
public:
	EditActionList() { };
	~EditActionList() { };

	void Reset() { actions.clear(); }
	void AddAction(const EditAction a) { actions.insert(a); };
	void AddAllAction() {
		AddAction(CAMERA_EDIT);
		AddAction(GEOMETRY_EDIT);
		AddAction(MATERIALS_EDIT);
	}
	bool Has(const EditAction a) const { return (actions.find(a) != actions.end()); };
	size_t Size() const { return actions.size(); };

	friend std::ostream &operator<<(std::ostream &os, const EditActionList &eal);
private:
	set<EditAction> actions;
};

inline std::ostream &operator<<(std::ostream &os, const EditActionList &eal) {
	os << "EditActionList[";

	bool sep = false;
	for (set<EditAction>::const_iterator it = eal.actions.begin(); it!=eal.actions.end(); ++it) {
		if (sep)
			os << ", ";

		switch (*it) {
			case CAMERA_EDIT:
				os << "CAMERA_EDIT";
				break;
			default:
				os << "UNKNOWN[" << *it << "]";
				break;
		}
	}

	os << "]";

	return os;
}

#endif	/* _SFERA_EDITACTION_H */
