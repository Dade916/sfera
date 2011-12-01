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

#include <stdexcept>

#include <boost/filesystem.hpp>

#include "sfera.h"
#include "gameconfig.h"
#include "displaysession.h"

void SferaDebugHandler(const char *msg) {
	cerr << "[Sfera] " << msg << endl;
}

#if defined(__GNUC__) && !defined(__CYGWIN__)
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <typeinfo>
#include <cxxabi.h>

static string Demangle(const char *symbol) {
	size_t size;
	int status;
	char temp[128];
	char* result;

	if (1 == sscanf(symbol, "%*[^'(']%*[^'_']%[^')''+']", temp)) {
		if (NULL != (result = abi::__cxa_demangle(temp, NULL, &size, &status))) {
			string r = result;
			return r + " [" + symbol + "]";
		}
	}

	if (1 == sscanf(symbol, "%127s", temp))
		return temp;

	return symbol;
}

void SferaTerminate(void) {
	SFERA_LOG("=========================================================");
	SFERA_LOG("Unhandled exception");

	void *array[32];
	size_t size = backtrace(array, 32);
	char **strings = backtrace_symbols(array, size);

	SFERA_LOG("Obtained " << size << " stack frames.");

	for (size_t i = 0; i < size; i++)
		SFERA_LOG("  " << Demangle(strings[i]));

	free(strings);
}
#endif

void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) {
	printf("\n*** ");
	if(fif != FIF_UNKNOWN)
		printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));

	printf("%s", message);
	printf(" ***\n");
}

int main(int argc, char *argv[]) {
#if defined(__GNUC__) && !defined(__CYGWIN__)
	set_terminate(SferaTerminate);
#endif

	try {
		SFERA_LOG("Usage: " << argv[0] << " [options] [configuration file]" << endl <<
				" -o [configuration file]" << endl <<
				" -w [window width]" << endl <<
				" -e [window height]" << endl <<
				" -D [property name] [property value]" << endl <<
				" -d [current directory path]" << endl <<
				" -h <display this help and exit>");

		// Initialize FreeImage Library
		FreeImage_Initialise(TRUE);
		FreeImage_SetOutputMessage(FreeImageErrorHandler);

		GameConfig *config = NULL;
		Properties cmdLineProp;
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == '-') {
				// I should check for out of range array index...

				if (argv[i][1] == 'h') exit(EXIT_SUCCESS);

				else if (argv[i][1] == 'o') {
					if (config)
						throw runtime_error("Used multiple configuration files");

					config = new GameConfig(argv[++i]);
				}

				else if (argv[i][1] == 'e') cmdLineProp.SetString("screen.height", argv[++i]);

				else if (argv[i][1] == 'w') cmdLineProp.SetString("screen.width", argv[++i]);

				else if (argv[i][1] == 'D') {
					cmdLineProp.SetString(argv[i + 1], argv[i + 2]);
					i += 2;
				}

				else if (argv[i][1] == 'd') boost::filesystem::current_path(boost::filesystem::path(argv[++i]));

				else {
					SFERA_LOG("Invalid option: " << argv[i]);
					exit(EXIT_FAILURE);
				}
			} else {
				string s = argv[i];
				if ((s.length() >= 4) && (s.substr(s.length() - 4) == ".cfg")) {
					if (config)
						throw runtime_error("Used multiple configuration files");
					config = new GameConfig(s);
				} else
					throw runtime_error("Unknown file extension: " + s);
			}
		}

		if (!config) {
			// Look for the default config

			if (boost::filesystem::exists("gamedata/cfgs/default.cfg")) {
				SFERA_LOG("Reading the default configuration file: gamedata/cfgs/default.cfg");
				config = new GameConfig("gamedata/cfgs/default.cfg");
			} else
				config = new GameConfig();
		}

		// Overtwirte properties with the one defined on command line
		config->LoadProperties(cmdLineProp);
		config->LogParameters();

		DisplaySession displaySession(config);
		displaySession.RunGame();

		delete config;
#if !defined(SFERA_DISABLE_OPENCL)
	} catch (cl::Error err) {
		SFERA_LOG("OpenCL ERROR: " << err.what() << "(" << OCLErrorString(err.err()) << ")");
#endif
	} catch (runtime_error err) {
		SFERA_LOG("RUNTIME ERROR: " << err.what());
	} catch (exception err) {
		SFERA_LOG("ERROR: " << err.what());
	}

	return EXIT_SUCCESS;
}
