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

#ifndef _SFERA_RENDERTEXT_H
#define	_SFERA_RENDERTEXT_H

#include "sfera.h"
#include "gameconfig.h"

class RenderText;

class RenderTextMenu {
public:
	RenderTextMenu(RenderText *rt,
			const string &header,
			const string &options,
			const string &footer,
			const int interline = 4);
	~RenderTextMenu();

	void Draw(const int scrWidth, const int scrHeight) const;

	int GetSelectedOption() const { return selectedOption; }
	void SetSelectedOption(const int i) { selectedOption = i; }

private:
	int interline;

	RenderText *renderText;

	vector<SDL_Surface *> header;
	vector<SDL_Surface *> options;
	vector<SDL_Surface *> footer;

	int headerSize, optionsSize;

	int selectedOption;
};

class RenderText {
public:
	RenderText(const GameConfig *cfg, TTF_Font *small, TTF_Font *medium, TTF_Font *big);
	~RenderText() { }

	// Single line text
	SDL_Surface *Create(const string &text) const;
	void Free(SDL_Surface *textSurf) const;
	void Draw(SDL_Surface *textSurf,
		const int x, const int y, const bool shadow = false) const;
	// An utility method
	void Draw(const string &text,
			const int x, const int y,
			const bool shadow = false) const {
		SDL_Surface *surf = Create(text);
		Draw(surf,x ,y, shadow);
		Free(surf);
	}

	// Multi-line text
	void Draw(const string &text, const bool shadow = false) const;

private:
	void Draw(const vector<string> &text, const bool shadow = false) const;
	void Print(SDL_Surface *textSurf,
		const int x, const int y) const;

	const GameConfig *gameConfig;
	TTF_Font *fontSmall, *fontMedium, *fontBig;
};

#endif	/* _SFERA_RENDERTEXT_H */

