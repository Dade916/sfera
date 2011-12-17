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

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "utils/utils.h"
#include "utils/properties.h"

Properties::Properties(const std::string &fileName) {
	LoadFile(fileName);
}

void Properties::Load(const Properties &p) {
	std::vector<std::string> keys = p.GetAllKeys();
	for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it)
		SetString(*it, p.GetString(*it, ""));
}

void Properties::LoadFile(const std::string &fileName) {
	std::ifstream file(fileName.c_str(), std::ios::in);
	if (file.fail())
		throw std::runtime_error("Unable to open file" + fileName);

	char buf[512];
	for (int lineNumber = 1;; ++lineNumber) {
		file.getline(buf, 512);
		if (file.eof())
			break;
		// Ignore comments
		if (buf[0] == '#')
			continue;

		std::string line = buf;
		boost::trim(line);

		// Ignore empty lines
		if (line.length() == 0)
			continue;

		size_t idx = line.find('=');
		if (idx == std::string::npos) {
			sprintf(buf, "Syntax error at line %d", lineNumber);
			throw std::runtime_error(buf);
		}

		// Check if it is a valid key
		std::string key(line.substr(0, idx));
		boost::trim(key);
		std::string value(line.substr(idx + 1));
		// Check if the last char is a LF or a CR and remove that (in case of
		// a DOS file red under Linux/MacOS)
		if ((value.size() > 0) && ((value[value.size() - 1] == '\n') || (value[value.size() - 1] == '\r')))
			value.resize(value.size() - 1);
		boost::trim(value);

		props[key] = value;
	}
}

void Properties::SaveFile(const std::string &fileName) {
	std::ofstream file(fileName.c_str(), std::ios::out);
	if (file.fail())
		throw std::runtime_error("Unable to open file" + fileName);

	std::vector<std::string> keys = GetAllKeys();
	for (size_t i = 0; i < keys.size(); ++i)
		file << keys[i] << "=" << GetString(keys[i], "0.0") << "\n";

	file.close();
}

std::vector<std::string> Properties::GetAllKeys() const {
	std::vector<std::string> keys;
	for (std::map<std::string, std::string>::const_iterator it = props.begin(); it != props.end(); ++it)
		keys.push_back(it->first);

	return keys;
}

std::vector<std::string> Properties::GetAllKeys(const std::string prefix) const {
	std::vector<std::string> keys;
	for (std::map<std::string, std::string>::const_iterator it = props.begin(); it != props.end(); ++it) {
		if (it->first.find(prefix) == 0)
			keys.push_back(it->first);
	}

	return keys;
}

bool Properties::IsDefined(const std::string propName) const {
	std::map<std::string, std::string>::const_iterator it = props.find(propName);

	if (it == props.end())
		return false;
	else
		return true;
}

std::string Properties::GetString(const std::string propName, const std::string defaultValue) const {
	std::map<std::string, std::string>::const_iterator it = props.find(propName);

	if (it == props.end())
		return defaultValue;
	else
		return it->second;
}

int Properties::GetInt(const std::string propName, const int defaultValue) const {
	std::string s = GetString(propName, "");

	if (s.compare("") == 0)
		return defaultValue;
	else
		return atoi(s.c_str());
}

float Properties::GetFloat(const std::string propName, const float defaultValue) const {
	std::string s = GetString(propName, "");

	if (s.compare("") == 0)
		return defaultValue;
	else
		return atof(s.c_str());
}

std::vector<std::string> Properties::GetStringVector(const std::string propName, const std::string &defaultValue) const {
	std::string s = GetString(propName, "");

	if (s.compare("") == 0)
		return ConvertToStringVector(defaultValue);
	else
		return ConvertToStringVector(s);
}

std::vector<int> Properties::GetIntVector(const std::string propName, const std::string &defaultValue) const {
	std::string s = GetString(propName, "");

	if (s.compare("") == 0)
		return ConvertToIntVector(defaultValue);
	else
		return ConvertToIntVector(s);
}

std::vector<float> Properties::GetFloatVector(const std::string propName, const std::string &defaultValue) const {
	std::string s = GetString(propName, "");

	if (s.compare("") == 0)
		return ConvertToFloatVector(defaultValue);
	else
		return ConvertToFloatVector(s);
}

void Properties::SetString(const std::string &propName, const std::string &value) {
	props[propName] = value;
}

std::string Properties::SetString(const std::string &property) {
	std::vector<std::string> strs;
	boost::split(strs, property, boost::is_any_of("="));

	if (strs.size() != 2)
		throw std::runtime_error("Syntax error in property definition");

	boost::trim(strs[0]);
	boost::trim(strs[1]);
	SetString(strs[0], strs[1]);

	return strs[0];
}

std::string Properties::ExtractField(const std::string &value, const size_t index) {
	char buf[512];
	memcpy(buf, value.c_str(), value.length() + 1);
	char *t = strtok(buf, ".");
	if ((index == 0) && (t == NULL))
		return value;

	size_t i = index;
	while (t != NULL) {
		if (i-- == 0)
			return std::string(t);
		t = strtok(NULL, ".");
	}

	return "";
}

std::vector<std::string>  Properties::ConvertToStringVector(const std::string &values) {
	std::vector<std::string> strs;
	boost::split(strs, values, boost::is_any_of("|"));

	std::vector<std::string> strs2;
	for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); ++it) {
		if (it->length() != 0)
			strs2.push_back(*it);
	}

	return strs2;
}

std::vector<int> Properties::ConvertToIntVector(const std::string &values) {
	std::vector<std::string> strs;
	boost::split(strs, values, boost::is_any_of("\t "));

	std::vector<int> ints;
	for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); ++it) {
		if (it->length() != 0)
			ints.push_back(atoi(it->c_str()));
	}

	return ints;
}

std::vector<float> Properties::ConvertToFloatVector(const std::string &values) {
	std::vector<std::string> strs;
	boost::split(strs, values, boost::is_any_of("\t "));

	std::vector<float> floats;
	for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); ++it) {
		if (it->length() != 0)
			floats.push_back(atof(it->c_str()));
	}

	return floats;
}

std::vector<float> Properties::GetParameters(const Properties &prop, const std::string &paramName,
		const unsigned int paramCount, const std::string &defaultValue) {
	const std::vector<float> vf = prop.GetFloatVector(paramName, defaultValue);
	if (vf.size() != paramCount) {
		std::stringstream ss;
		ss << "Syntax error in " << paramName << " (required " << paramCount << " parameters)";
		throw std::runtime_error(ss.str());
	}

	return vf;
}
