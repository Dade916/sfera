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

#include "sfera.h"
#include "displaysession.h"
#include "gamesession.h"
#include "renderer/cpu/singlecpurenderer.h"
#include "renderer/cpu/multicpurenderer.h"
#include "renderer/ocl/oclrenderer.h"
#include "physic/gamephysic.h"
#include "sdl/editaction.h"
#include "utils/packlist.h"

static const string SFERA_LABEL = "Sfera v" SFERA_VERSION_MAJOR "." SFERA_VERSION_MINOR " (Written by David \"Dade\" Bucciarelli)";

DisplaySession::DisplaySession(const GameConfig *cfg) : gameConfig(cfg) {
	leftMouseDown = false;
	rightMouseDown = false;
	mouseStartX = 0;
	mouseStartY = 0;
	startViewTheta = 0.f;
	startViewPhi = 0.f;

	const unsigned int width = gameConfig->GetScreenWidth();
	const unsigned int height = gameConfig->GetScreenHeight();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw runtime_error("Unable to initialize SDL");
	if (TTF_Init() < 0)
		throw runtime_error("Unable to initialize SDL TrueType library");

	// Text fonts
	fontSmall = TTF_OpenFont(gameConfig->GetScreenFontName().c_str(), gameConfig->GetScreenFontSize());
	if (!fontSmall)
		throw runtime_error("Unable to open " + gameConfig->GetScreenFontName() + " small font file. Error: " + TTF_GetError());
	fontMedium = TTF_OpenFont(gameConfig->GetScreenFontName().c_str(), 2 * gameConfig->GetScreenFontSize());
	if (!fontMedium)
		throw runtime_error("Unable to open " + gameConfig->GetScreenFontName() + " medium font file. Error: " + TTF_GetError());
	fontBig = TTF_OpenFont(gameConfig->GetScreenFontName().c_str(), 3 * gameConfig->GetScreenFontSize());
	if (!fontBig)
		throw runtime_error("Unable to open " + gameConfig->GetScreenFontName() + " big font file. Error: " + TTF_GetError());

	renderText = new RenderText(gameConfig, fontSmall, fontMedium, fontBig);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);

	SDL_putenv((char *)"SDL_VIDEO_CENTERED=center");
	SDL_WM_SetCaption(SFERA_LABEL.c_str(), NULL);

	screenSurface = SDL_SetVideoMode(width, height, 32,
			SDL_HWSURFACE | SDL_OPENGL);
	if (!screenSurface)
		throw runtime_error("Unable to create SDL window");

	glewInit();
	if (!glewIsSupported("GL_VERSION_2_0 " "GL_ARB_pixel_buffer_object"))
		throw runtime_error("GL_ARB_pixel_buffer_object is not supported");

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, width, height);
	glLoadIdentity();
	glOrtho(0.f, width - 1.f,
			0.f, height - 1.f, -1.f, 1.f);
}

DisplaySession::~DisplaySession() {
	TTF_CloseFont(fontSmall);
	TTF_CloseFont(fontMedium);
	TTF_CloseFont(fontBig);

	TTF_Quit();
	SDL_Quit();
}

//------------------------------------------------------------------------------
// Run related methods
//------------------------------------------------------------------------------

void DisplaySession::DrawLevelLabels(const string &bottomLabel, const string &topLabel) const {

	SDL_Surface *bottomLabelSurf = renderText->Create(bottomLabel);
	SDL_Surface *topLabelSurf = renderText->Create(topLabel);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.f, 0.f, 0.f, 0.5f);
	if (bottomLabelSurf)
		glRecti(0, 0,
				gameConfig->GetScreenWidth() - 1, bottomLabelSurf->h);
	if (topLabelSurf)
		glRecti(0, gameConfig->GetScreenHeight() - topLabelSurf->h - 1,
				gameConfig->GetScreenWidth() - 1, gameConfig->GetScreenHeight() - 1);

	glColor4f(1.0f, 1.0f, 1.0f, 1.f);
	if (bottomLabelSurf)
		renderText->Draw(bottomLabelSurf, 0, 0);
	if (topLabelSurf)
		renderText->Draw(topLabelSurf, 0, gameConfig->GetScreenHeight() - topLabelSurf->h - 1);
	glDisable(GL_BLEND);

	renderText->Free(bottomLabelSurf);
	renderText->Free(topLabelSurf);
}

bool DisplaySession::RunLevel(GameSession &gameSession) {
	GameLevel *currentLevel = gameSession.currentLevel;

	GamePhysic gamePhysic(currentLevel);
	PhysicThread physicThread(&gamePhysic);

	LevelRenderer *renderer;
	switch (gameConfig->GetRendererType()) {
		case SINGLE_CPU:
			renderer = new SingleCPURenderer(currentLevel);
			break;
		case MULTI_CPU:
			renderer = new MultiCPURenderer(currentLevel);
			break;
		case OPENCL:
			renderer = new OCLRenderer(currentLevel);
			break;
		default:
			throw runtime_error("Unknown rendrer type: " + gameConfig->GetRendererType());
			break;
	}

	//--------------------------------------------------------------------------
	// Start the game
	//--------------------------------------------------------------------------

	physicThread.Start();

	unsigned int frame = 0;
	double frameStartTime = WallClockTime();
	unsigned long long totalSampleCount = 0;
	const double refreshTime = 1.0 / gameConfig->GetScreenRefreshCap();
	double currentRefreshTime = 0.0;
	string topLabel = "";
	string bottomLabel = "";

	bool quit = false;
	bool levelDone = false;
	double lastJumpTime = 0.0;

	do {
		const double t1 = WallClockTime();

		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN: {
					switch(event.button.button) {
						case SDL_BUTTON_LEFT: {
							leftMouseDown = true;
							mouseStartX = event.button.x;
							mouseStartY = event.button.y;
							startViewTheta = currentLevel->player->viewTheta;
							startViewPhi = currentLevel->player->viewPhi;
							break;
						}
						case SDL_BUTTON_RIGHT: {
							rightMouseDown = true;
							mouseStartX = event.button.x;
							mouseStartY = event.button.y;
							startViewDistance = currentLevel->player->viewDistance;
							break;
						}
					}
					break;
				}
				case SDL_MOUSEBUTTONUP: {
					switch(event.button.button) {
						case SDL_BUTTON_LEFT: {
							leftMouseDown = false;
							break;
						}
						case SDL_BUTTON_RIGHT: {
							rightMouseDown = false;
							break;
						}
					}
					break;
				}
				case SDL_MOUSEMOTION: {
					if (leftMouseDown) {
						const int deltaX = event.motion.x - mouseStartX;
						const int deltaY = event.motion.y - mouseStartY;

						currentLevel->player->viewTheta =  startViewTheta - deltaY / 200.f;
						currentLevel->player->viewPhi = startViewPhi - deltaX / 200.f;
					}

					if (rightMouseDown) {
						const int deltaY = event.motion.y - mouseStartY;

						currentLevel->player->viewDistance =  Max(
								currentLevel->player->body.sphere.rad * 5.f,
								Min(currentLevel->player->body.sphere.rad * 40.f,
									startViewDistance + deltaY / 100.f));
					}
					break;
				}
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_w:
							currentLevel->player->inputGoForward = true;
							break;
						case SDLK_a:
							currentLevel->player->inputTurnLeft = true;
							break;
						case SDLK_d:
							currentLevel->player->inputTurnRight = true;
							break;
						case SDLK_s:
							currentLevel->player->inputSlowDown = true;
							break;
						case SDLK_SPACE: {
							const double now = WallClockTime();
							if (now - lastJumpTime > 0.5) { // Not more often than every 0.5 secs
								currentLevel->player->inputJump = true;
								lastJumpTime = now;
							}
							break;
						}
						default:
							break;
					}
					break;
				}
				case SDL_KEYUP: {
					switch (event.key.keysym.sym) {
						case SDLK_w:
							currentLevel->player->inputGoForward = false;
							break;
						case SDLK_a:
							currentLevel->player->inputTurnLeft = false;
							break;
						case SDLK_d:
							currentLevel->player->inputTurnRight = false;
							break;
						case SDLK_s:
							currentLevel->player->inputSlowDown = false;
							break;
						case SDLK_ESCAPE:
							quit = true;
							break;
						default:
							break;
					}
					break;
				}
				case SDL_QUIT:
					quit = true;
					break;
				default:
					break;
			}
		}

		totalSampleCount += renderer->DrawFrame();

		//----------------------------------------------------------------------
		// Draw text
		//----------------------------------------------------------------------

		DrawLevelLabels(bottomLabel, topLabel);

		if (WallClockTime() - currentLevel->startTime < 3.0) {
			stringstream ss;
			ss << "[font=big]Level " << gameSession.GetCurrentLevel() << "\n[font=big]" << gameSession.GetCurrentLevelName();
			renderText->Draw(ss.str(), true);
		}

		SDL_GL_SwapBuffers();

		//----------------------------------------------------------------------
		// Sleep if we are going too fast
		//----------------------------------------------------------------------

		currentRefreshTime = WallClockTime() - t1;
		if (currentRefreshTime < refreshTime) {
			const unsigned int sleep = (unsigned int)((refreshTime - currentRefreshTime) * 1000.0);
			boost::this_thread::sleep(boost::posix_time::millisec(sleep));
		}

		//----------------------------------------------------------------------
		// Update the bottom and top label
		//----------------------------------------------------------------------

		const double now = WallClockTime();

		stringstream ss;
		ss << "[Lights " << currentLevel->offPillCount << "/" << currentLevel->scene->pillCount <<
				"][Time " << fixed << setprecision(2) << now - currentLevel->startTime << "secs]";
		bottomLabel = ss.str();

		++frame;
		const double dt = now - frameStartTime;
		if (dt > 1.0) {
			const double now = WallClockTime();

			const double frameSec = frame / dt;
			const double sampleSec = totalSampleCount / dt;

			stringstream ss;
			ss << "[Sample/sec: " << fixed << setprecision(2) << (sampleSec / 100000) <<
					"M s/s][Frame/sec: " << frameSec <<	"/" << gameConfig->GetScreenRefreshCap() <<
					"][Physic engine Hz: " << gamePhysic.GetRunningHz() <<
					"/"<< gameConfig->GetPhysicRefreshRate() << "]";
			topLabel = ss.str();

			frameStartTime = now;
			frame = 0;
			totalSampleCount = 0;
		}

		//----------------------------------------------------------------------
		// Check If I have turned off all pills
		//----------------------------------------------------------------------

		if (currentLevel->offPillCount >= currentLevel->scene->pillCount) {
			// Done, show the points and exit
			levelDone = true;
			quit = true;
		}
	} while(!quit);

	// Freeze the world
	physicThread.Stop();

	//--------------------------------------------------------------------------
	// Show the end of level result
	//--------------------------------------------------------------------------

	stringstream ss;
	if (levelDone) {
		// Well done
		const double secs = WallClockTime() - currentLevel->startTime;
		ss << "[font=big]Well done: " << fixed << setprecision(2) << secs << " secs";
		gameSession.AddLevelTime(secs);
	} else {
		// Game over
		ss << "[font=big]Game Over";
	}
	const string msg = ss.str();

	// Wait for a key/mouse event
	const double startTime = WallClockTime();
	for (bool endWait = false; !endWait;) {
		renderer->DrawFrame();

		renderText->Draw(msg, true);

		SDL_GL_SwapBuffers();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
				case SDL_KEYDOWN:
					// Wait at least 1sec
					if (WallClockTime() - startTime > 1.0)
						endWait = true;
					break;
			}
		}

		boost::this_thread::sleep(boost::posix_time::millisec(100));
	}

	delete renderer;

	return levelDone;
}

bool DisplaySession::RunIntro() {
	const string introMsg = "[font=big]Sfera v" SFERA_VERSION_MAJOR "." SFERA_VERSION_MINOR "\n[font=medium] \n"
		"[font=medium]Written by\n[font=medium]David \"Dade\" Bucciarelli";

	bool quit = false;
	for(;;) {
		// Wait for a key/mouse event
		for (bool endWait = false; !endWait;) {
			glColor3f(0.0f, 0.0f, 0.0f);
			glRecti(0, 0,
					gameConfig->GetScreenWidth() - 1, gameConfig->GetScreenHeight() - 1);

			renderText->Draw(introMsg, true);

			SDL_GL_SwapBuffers();

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
					case SDL_KEYDOWN: {
						switch (event.key.keysym.sym) {
							case SDLK_ESCAPE:
								quit = true;
								endWait = true;
								break;
							default:
								endWait = true;
								break;
						}
						break;
					}
					case SDL_QUIT:
						quit = true;
						endWait = true;
						break;
					case SDL_MOUSEBUTTONDOWN:
						endWait = true;
						break;
				}
			}

			boost::this_thread::sleep(boost::posix_time::millisec(100));
		}

		break;
	}

	return quit;
}

void DisplaySession::RunGameSession(const string &pack) {
	GameSession gameSession(gameConfig, pack);
	gameSession.Begin();

	for(;;) {
		if (RunLevel(gameSession)) {
			if (!gameSession.NextLevel())
				break;
		} else
			break;
	}

	// Show the final score
	stringstream ss;
	ss << "[font=big]Level " << gameSession.GetCurrentLevel() - 1 <<
			"\n[font=big]Total time " << fixed << setprecision(2) << gameSession.GetTotalLevelsTime() << " secs";
	const string msg = ss.str();

	// Wait for a key/mouse event
	const double startTime = WallClockTime();
	for (bool endWait = false; !endWait;) {
		glColor3f(0.f, 0.f, 0.f);
		glRecti(0, 0,
				gameConfig->GetScreenWidth() - 1, gameConfig->GetScreenHeight() - 1);

		renderText->Draw(msg, true);

		SDL_GL_SwapBuffers();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
				case SDL_KEYDOWN:
					// Wait at least 1sec
					if (WallClockTime() - startTime > 1.0)
						endWait = true;
					break;
			}
		}

		boost::this_thread::sleep(boost::posix_time::millisec(100));
	}

	SFERA_LOG("Game Session is over");
}

bool DisplaySession::RunPackSelection(string *pack) {
	const unsigned int interline = 4;

	vector<SDL_Surface *> header;
	header.push_back(renderText->Create("[font=medium] Select a level pack:"));
	header.push_back(renderText->Create("[font=medium] "));
	const unsigned int headerSize = header[0]->h + header[1]->h + interline;

	// Add the list of packs
	PackList packList;
	vector<SDL_Surface *> options;
	size_t selected = 0;
	unsigned int optionsSize = interline * packList.names.size() - 1;
	for (size_t i = 0; i < packList.names.size(); ++i) {
		if ("Sfera" == packList.names[i])
			selected = i;

		options.push_back(renderText->Create("[font=medium]" + packList.names[i]));
		optionsSize += options[i]->h;
	}

	vector<SDL_Surface *> footer;
	footer.push_back(renderText->Create("[font=medium] "));
	footer.push_back(renderText->Create("[font=small](Up/Down to scroll, press the space bar to select)"));

	bool quit = false;
	const unsigned int width = gameConfig->GetScreenWidth();
	const unsigned int height = gameConfig->GetScreenHeight();
	for(;;) {
		// Wait for a key/mouse event
		for (bool endWait = false; !endWait;) {
			// Background
			glColor3f(0.0f, 0.0f, 0.0f);
			glRecti(0, 0, width, height);

			// Selection box
			glColor3f(0.0f, 0.0f, 1.0f);
			unsigned int offset = (height - options[selected]->h) / 2;
			glRecti((width - options[selected]->w) / 2 - 1, offset - 1,
					(width - options[selected]->w) / 2 + options[selected]->w + 1, offset + options[selected]->h + 1);

			// Draw the menu
			glColor3f(1.0f, 1.0f, 1.0f);
			for (size_t i = 0; i < selected; ++i)
				offset += options[selected]->h + interline;
			offset += headerSize;

			// Draw header
			for (size_t i = 0; i < header.size(); ++i) {
				renderText->Draw(header[i],
						(width - header[i]->w) / 2, offset,
						true);

				offset -= options[i]->h + interline;
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

			SDL_GL_SwapBuffers();

			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
					case SDL_KEYDOWN: {
						switch (event.key.keysym.sym) {
							case SDLK_ESCAPE:
								quit = true;
								endWait = true;
								break;
							case SDLK_UP:
								selected = Max<size_t>(0, selected - 1);
								break;
							case SDLK_DOWN:
								selected = Min<size_t>(options.size() - 1, selected + 1);
								break;
							default:
								endWait = true;
								break;
						}
						break;
					}
					case SDL_QUIT:
						quit = true;
						endWait = true;
						break;
				}
			}

			boost::this_thread::sleep(boost::posix_time::millisec(100));
		}

		break;
	}

	*pack = packList.names[selected];
	return quit;
}

void DisplaySession::RunGame() {
	bool quit = false;

	do {
		if (RunIntro()) {
			// I have to quit
			quit = true;
		}

		if (!quit) {
			string pack;
			if (RunPackSelection(&pack)) {
				// I have to quit
				quit = true;
			}

			if (!quit)
				RunGameSession(pack);
		}
	} while (!quit);

	SFERA_LOG("Done.");
}
