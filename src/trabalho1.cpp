/*
	Name: test01.2.Two.Windows.cpp
	Author: Rodrigo Luis de Souza da Silva
	Author: Bruno José Dembogurski
	Date: 27/01/2011
	Last Update: 27/01/2016
	Description: Managing two windows using PixelLab.
*/

#include "pixelLab.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "GL/glut.h"
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <vector>

#define CWIDTH 256
#define CHEIGHT 256

#define BOX_SIZE 232

#define DASHED_AMOUNT 4

#define BOX_LEFT (CWIDTH - BOX_SIZE)
#define BOX_RIGHT CWIDTH
#define BOX_TOP 0
#define BOX_BOTTOM BOX_SIZE

#define DASHED_SPACING (BOX_SIZE / DASHED_AMOUNT)

typedef struct {
	float x;
	float y;
} Point;

// Image Objects
PixelLab *img = NULL, *imgOriginal = NULL;
int window1 = 0;
int window2 = 0;
int brightnessSlider = 128;

std::vector<Point> pts;

void idle() {
	glutPostRedisplay();
}

static void display1(void) {
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// reverse 'Y' to allow correct mouse click (0 value is in the upper left corner for mouse functions)
	gluOrtho2D(0, CWIDTH, CHEIGHT, 0); // You could use "glOrtho(0, 64, 256, 0, -5, 5);" instead if you want 3D features

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// draw left vertical color box
	glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2d(0, 0);
		glVertex2d(16, 0);
		glVertex2d(16, 232);
		glVertex2d(0, 232);
	glEnd();

	glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2d(1, 1);
		glVertex2d(15, 1);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex2d(15, 231);
		glVertex2d(1, 231);
	glEnd();

	// draw bottom horizontal color box
		glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2d(24, 240);
		glVertex2d(24, 256);
		glVertex2d(256, 256);
		glVertex2d(256, 240);
	glEnd();

	glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex2d(25, 241);
		glVertex2d(25, 255);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2d(255, 255);
		glVertex2d(255, 241);
	glEnd();

	// draw center control box
	glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2d(24, 0);
		glVertex2d(256, 0);
		glVertex2d(256, 232);
		glVertex2d(24, 232);
	glEnd();

	glBegin(GL_QUADS);
		glColor3f(0.94f, 0.94f, 0.94f);
		glVertex2d(25, 1);
		glVertex2d(255, 1);
		glVertex2d(255, 231);
		glVertex2d(25, 231);
	glEnd();

	// draw dashed lines
	glEnable(GL_LINE_STIPPLE);
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineStipple(2, 0xAAAA);
	glLineWidth(1);

	// vertical dashed lines
	for (int i = 1; i < DASHED_AMOUNT; ++i) {
		glBegin(GL_LINES);
			glVertex2f(BOX_LEFT + (i * DASHED_SPACING), BOX_TOP);
			glVertex2f(BOX_LEFT + (i * DASHED_SPACING), BOX_BOTTOM);
		glEnd();
	}

	// horizontal dashed lines
	for (int i = 1; i < DASHED_AMOUNT; ++i) {
		glBegin(GL_LINES);
			glVertex2f(BOX_LEFT, BOX_TOP + (i * DASHED_SPACING));
			glVertex2f(BOX_RIGHT, BOX_TOP + (i * DASHED_SPACING));
		glEnd();
	}

	glDisable(GL_LINE_STIPPLE);

	// draw default curve
	float fx = 0.0f;
	float dy = 0.0f;
	int i = 0;

	glBegin(GL_LINE_STRIP);
		glColor3f(0.0f, 0.0f, 0.0f);

		for (int x = 0; x < 256; ++x) {
			while(x > pts[i].x) ++i;

			if (!i) dy = pts[i].x? pts[i].y / pts[i].x : 0;
			else dy = (pts[i].y - pts[i - 1].y) / (pts[i].x - pts[i - 1].x);

			fx += dy;

			glVertex2f(BOX_LEFT + (x / 256.0f) * BOX_SIZE,
					BOX_BOTTOM - (fx / 256.0f) * BOX_SIZE);
		}
	glEnd();
	glutSwapBuffers();
}

static void display2(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	img->ViewImage();
	glutSwapBuffers();
}

void modifyImage() {
	int value = brightnessSlider - 128; // To make the middle of the slider, zero
	int pixelVal;

	for (int y = 0; y < imgOriginal->GetHeight(); y++) {
		for (int x = 0; x < imgOriginal->GetWidth(); x++) {
			pixelVal = imgOriginal->GetGrayValue(x, y);
			pixelVal += value;
			if (pixelVal >= 255) pixelVal = 255;
			if (pixelVal < 0) pixelVal = 0;
			img->SetGrayValue(x, y, (uByte) pixelVal);
		}
	}

	// Update both windows
	glutPostWindowRedisplay	(window1);
	glutPostWindowRedisplay	(window2);
}

// Keyboard
static void key(unsigned char key, int x, int y) {
	 switch (key) {
		case 27 :

		case 'q':
			exit(0);
		break;
	 }

	 glutPostRedisplay();
}

// Special Keys callback
void specialKeys(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_UP:
			brightnessSlider = (brightnessSlider<0) ? 0 : brightnessSlider-1;
			modifyImage();
		break;

		case GLUT_KEY_DOWN:
			brightnessSlider = (brightnessSlider>255) ? 255 : brightnessSlider+1;
			modifyImage();
		break;

		case 'q':
			exit(0);
	}

	glutPostRedisplay();
}

// Mouse callback - Capture mouse click in the brightness window
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON)
		if (state == GLUT_DOWN)
			brightnessSlider = y;
	modifyImage();
}

// Motion callback - Capture mouse motion when left button is clicked
void motion(int x, int y ) {
	brightnessSlider = y;
	modifyImage();
}

void init() {
	imgOriginal = new PixelLab();
	imgOriginal->Read("figs/lenaGray.png");

	Point p1 = {0.0f, 0.0f};
	pts.push_back(p1);

	Point p2 = {0.3f * 256, 0.6f * 256};
	pts.push_back(p2);

	Point p3 = {0.6f * 256, 0.3f * 256};
	pts.push_back(p3);

	Point pn = {256.0f, 256.0f};
	pts.push_back(pn);

	img = new PixelLab();
	img->Copy(imgOriginal);

	printf("Change brightness clicking on the left window\n or using the 'up' and 'down' keyboard keys.\n");
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	// Init image objects
	init();

	// Create two windows
	window2 = glutCreateWindow("Display Window");
	window1 = glutCreateWindow("Control Window");

	///////////////////////////////////////////////////////////
	// Control Window
	glutSetWindow(window1);	// Change current window to 1 (all callbacks will be set to this window)
	glutDisplayFunc(display1);
	glutPositionWindow(20, 30);
	glutReshapeWindow(CWIDTH, CHEIGHT);
	//glutMouseFunc(mouse);
	//glutMotionFunc(motion);
	//glutSpecialFunc(specialKeys);
	glutKeyboardFunc(key);

	///////////////////////////////////////////////////////////
	// Display Window
	glutSetWindow(window2); // Change current window to 2
	glutDisplayFunc(display2);
	glutReshapeWindow(imgOriginal->GetWidth(),imgOriginal->GetHeight());
	glutPositionWindow(25 + CWIDTH, 30);
	glutKeyboardFunc(key);

	glutMainLoop();

	return 0;
}
