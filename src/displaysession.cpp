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

	font = TTF_OpenFont(gameConfig->GetScreenFontName().c_str(), gameConfig->GetScreenFontSize());
	if (!font)
		throw runtime_error("Unable to open " + gameConfig->GetScreenFontName() + " font file");

	SDL_Color white = { 255, 255, 255 };
	SDL_Surface *fontSizeTest = TTF_RenderText_Blended(font, "Test 123", white);
	fontSize = fontSizeTest->h;
	SDL_FreeSurface(fontSizeTest);

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
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();
}

void DisplaySession::RenderText(const string &text, const unsigned int x, const unsigned int y) const {
	if (text.length() == 0)
		return;

	static SDL_Color white = { 255, 255, 255 };
	SDL_Surface *initial = TTF_RenderText_Blended(font, text.c_str(), white);

	const int w = RoundUpPow2(initial->w);
	const int h = RoundUpPow2(initial->h);
	SDL_Surface *intermediary = SDL_CreateRGBSurface(0, w, h, 32,
			0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

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
	glVertex2f(x0, y0);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(x1, y0);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(x1, y1);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(x0, y1);
	glEnd();
	glFinish();

	SDL_FreeSurface(initial);
	SDL_FreeSurface(intermediary);
	glDeleteTextures(1, (GLuint *)&texture);
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

		// Draw text
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.f, 0.f, 0.f, 0.5f);
		glRecti(0, 0,
				gameConfig->GetScreenWidth() - 1, fontSize);
		glRecti(0, gameConfig->GetScreenHeight() - fontSize - 1,
				gameConfig->GetScreenWidth() - 1, gameConfig->GetScreenHeight() - 1);

		glBlendFunc(GL_ONE, GL_ONE);
		glColor3f(1.0f, 1.0f, 1.0f);
		RenderText(bottomLabel, 0, 0);
		RenderText(topLabel, 0, gameConfig->GetScreenHeight() - fontSize - 1);
		glDisable(GL_BLEND);

		SDL_GL_SwapBuffers();

		const double now = WallClockTime();
		// Update the bottom label
		stringstream ss;
		ss << "[Spheres " << currentLevel->offPillCount << "/" << currentLevel->scene->pillCount <<
				"][Time " << fixed << setprecision(2) << now - currentLevel->startTime << "secs]";
		bottomLabel = ss.str();

		currentRefreshTime = now - t1;
		if (currentRefreshTime < refreshTime) {
			const unsigned int sleep = (unsigned int)((refreshTime - currentRefreshTime) * 1000.0);
			boost::this_thread::sleep(boost::posix_time::millisec(sleep));
		}

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

		// Check If I have turned off all pills
		if (currentLevel->offPillCount >= currentLevel->scene->pillCount) {
			levelDone = true;
			quit = true;
		}
	} while(!quit);

	delete renderer;
	physicThread.Stop();

	return levelDone;
}

void DisplaySession::RunLoop() {
	GameSession gameSession(gameConfig, "Sfera");
	gameSession.Begin(2);

	for(;;) {
		if (RunLevel(gameSession)) {
			if (!gameSession.NextLevel())
				break;
		} else
			break;
	}

	SFERA_LOG("Done.");
}
