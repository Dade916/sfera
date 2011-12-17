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

#include "utils/rendertext.h"

RenderText::RenderText(const GameConfig *cfg, TTF_Font *small,	TTF_Font *medium, TTF_Font *big) {
	gameConfig = cfg;
	fontSmall = small;
	fontMedium = medium;
	fontBig = big;
}

void RenderText::Print(SDL_Surface *initial,
		const int x, const int y) const {
	if (!initial)
		return;

	const int w = RoundUpPow2(initial->w);
	const int h = RoundUpPow2(initial->h);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const Uint32 rmask = 0xff000000;
    const Uint32 gmask = 0x00ff0000;
    const Uint32 bmask = 0x0000ff00;
    const Uint32 amask = 0x000000ff;
#else
    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;
#endif

	SDL_Surface *intermediary = SDL_CreateRGBSurface(0, w, h, 32,
			rmask, gmask, bmask, amask);

	SDL_Rect dest;
	dest.x = 0;
	dest.y = h - initial->h;
	dest.w = initial->w;
	dest.h = initial->h;
	SDL_BlitSurface(initial, 0, intermediary, &dest);

	GLuint texture;
	glGenTextures(1, (GLuint *)&texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_BGRA,
			GL_UNSIGNED_BYTE, intermediary->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);

	const int x0 = x;
	const int y0 = y;
	const int x1 = x + w;
	const int y1 = y + h;

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2i(x0, y0);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2i(x1, y0);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2i(x1, y1);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(x0, y1);
	glEnd();
	glFinish();

	SDL_FreeSurface(intermediary);
	glDeleteTextures(1, (GLuint *)&texture);
}

SDL_Surface *RenderText::Create(const string &text) const {
	static SDL_Color white = { 255, 255, 255 };

	if (text.length() == 0)
		return NULL;

	// Pick the font to use
	if (boost::starts_with(text, "[font=big]"))
		return TTF_RenderText_Solid(fontBig, text.c_str() + 10, white);
	else if (boost::starts_with(text, "[font=medium]"))
		return TTF_RenderText_Solid(fontMedium, text.c_str() + 13, white);
	else if (boost::starts_with(text, "[font=small]"))
		return TTF_RenderText_Solid(fontSmall, text.c_str() + 12, white);
	else
		return TTF_RenderText_Solid(fontSmall, text.c_str(), white);
}

void RenderText::Free(SDL_Surface *textSurf) const {
	SDL_FreeSurface(textSurf);
}

void RenderText::Draw(SDL_Surface *textSurf,
			const int x, const int y,
		const bool shadow) const {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (shadow) {
		glColor4f(0.25f, 0.25f, 0.25f, 1.f);
		Print(textSurf, x + 2, y);
		glColor4f(1.0f, 1.f, 1.f, 1.f);
		Print(textSurf, x, y + 2);
	} else {
		glColor4f(1.0f, 1.f, 1.f, 1.f);
		Print(textSurf, x , y);
	}

	glDisable(GL_BLEND);
}

void RenderText::Draw(const string &text, const bool shadow) const {
	// Check if it is a multi-line message
	if (text.find("\n") != string::npos) {
		vector<std::string> msgs;
		boost::split(msgs, text, boost::is_any_of("\n"));
		Draw(msgs, shadow);
	} else {
		SDL_Surface *surf = Create(text);
		const int x = (int(gameConfig->GetScreenWidth()) - surf->w) / 2;
		const int y = (int(gameConfig->GetScreenHeight()) - surf->h) / 2;

		Draw(surf , x, y, shadow);

		Free(surf);
	}
}

void RenderText::Draw(const vector<string> &texts, const bool shadow) const {
	const size_t size = texts.size();
	if (size == 0)
		return;

	vector<SDL_Surface *> surfs(texts.size());
	for (size_t i = 0; i < size; ++i)
		surfs[i] = Create(texts[i]);

	// The space between lines (i.e. 4 pixels)
	int totHeight = (size - 1) * 4;
	for (size_t i = 0; i < size; ++i)
		totHeight += surfs[i]->h;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int y = (int(gameConfig->GetScreenHeight()) - totHeight) / 2;
	for (size_t i = 0; i < size; ++i) {
		size_t index = size - 1 - i;
		const int x = (int(gameConfig->GetScreenWidth()) - surfs[index]->w) / 2;

		Draw(surfs[index] , x, y, shadow);

		y += surfs[index]->h + 4;
	}
	glDisable(GL_BLEND);

	for (size_t i = 0; i < size; ++i)
		Free(surfs[i]);
}

//------------------------------------------------------------------------------
// Text menu related methods
//------------------------------------------------------------------------------

RenderTextMenu::RenderTextMenu(RenderText *rt,
			const string &hdr, const string &opts, const string &ftr,
			const int interln) {
	renderText = rt;
	interline = interln;
	selectedOption = 0;

	// The menu header

	vector<std::string> headerMsgs;
	boost::split(headerMsgs, hdr, boost::is_any_of("\n"));

	headerSize = 0;
	for (size_t i = 0; i < headerMsgs.size(); ++i) {
		header.push_back(renderText->Create(headerMsgs[i]));
		headerSize += header[i]->h;
	}
	headerSize += interline * header.size();

	// The menu options

	vector<std::string> optionsMsgs;
	boost::split(optionsMsgs, opts, boost::is_any_of("\n"));
	optionsSize = 0;
	for (size_t i = 0; i < optionsMsgs.size(); ++i) {
		options.push_back(renderText->Create(optionsMsgs[i]));
		optionsSize += options[i]->h;
	}
	optionsSize += interline * options.size();

	// The menu footer

	vector<std::string> footerMsgs;
	boost::split(footerMsgs, ftr, boost::is_any_of("\n"));

	for (size_t i = 0; i < footerMsgs.size(); ++i)
		footer.push_back(renderText->Create(footerMsgs[i]));
}

RenderTextMenu::~RenderTextMenu() {
	for (size_t i = 0; i < header.size(); ++i)
		renderText->Free(header[i]);
	for (size_t i = 0; i < options.size(); ++i)
		renderText->Free(options[i]);
	for (size_t i = 0; i < footer.size(); ++i)
		renderText->Free(footer[i]);
}

void RenderTextMenu::Draw(const int width, const int height) const {
	// Selection box
	int offset = (height - options[selectedOption]->h) / 2;
	glColor3f(0.0f, 0.0f, 1.0f);
	glRecti((width - options[selectedOption]->w) / 2 - 1, offset - 1,
			(width - options[selectedOption]->w) / 2 + options[selectedOption]->w + 1, offset + options[selectedOption]->h + 1);

	//--------------------------------------------------------------------------
	// Draw the menu
	//--------------------------------------------------------------------------

	for (size_t i = 0; i < size_t(selectedOption); ++i)
		offset += options[selectedOption]->h + interline;
	offset += headerSize;

	// Draw header
	for (size_t i = 0; i < header.size(); ++i) {
		renderText->Draw(header[i],
				(width - header[i]->w) / 2, offset,
				true);

		offset -= header[i]->h + interline;
	}

	// Draw options
	for (size_t i = 0; i < options.size(); ++i) {
		renderText->Draw(options[i],
				(width - options[i]->w) / 2, offset,
				false);

		offset -= options[i]->h + interline;
	}

	// Draw footer
	for (size_t i = 0; i < footer.size(); ++i) {
		renderText->Draw(footer[i],
				(width - footer[i]->w) / 2, offset,
				true);

		offset -= footer[i]->h + interline;
	}
}
