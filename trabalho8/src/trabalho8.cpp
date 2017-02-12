#include "pixelLab.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <iostream>
#include <vector>
#include <tuple>
#include <utility>
#include <cmath>
#include <ctime>
#include <algorithm>
#include "GL/glui.h"
#include "GL/glut.h"
#include "Debug.h"

int MAX_ITER = 10;
int MIN_DIFF = 10;

using std::cout;
using std::endl;

// Image Objects
PixelLab *img = NULL;
PixelLab *imgMod = NULL;

GLUI_RadioGroup *radio, *radio2;
GLUI_RadioButton *buttons[3];
GLUI_Button *autoThresholdButton, *manualThresholdButton;
GLUI_Spinner *noiseSpinner, *spinner;

int orig_window, edited_window;
int UI_width = 0;
int option = 0;
int option2 = 0;
int filterRadio = 128;
float noiseProb = 0.05;
int prevBrightness = 0;

char input[512];
char output[512];

bool isTransformed = false;

class color {

public:
	color(uByte _r, uByte _g, uByte _b, uByte _a = 0xFF) :
			r(_r), g(_g), b(_b), a(_a) {
	}
	color(uByte _value) :
			color(_value, _value, _value) {
	}

	color() :
			color(0) {
	}

	uByte r;
	uByte g;
	uByte b;
	uByte a;
};

class seed {

public:
	int x, y;
	uByte min;
	uByte max;
	color c;
};

//std::vector<seed> seedVector { { 0, 0, 0xFF/2, 0xFF, color(0xFF) } };
std::vector<seed> seedVector { { 0, 0, 0, 0xFF/2, color(0xFF) } };

void idle() {
	if (glutGetWindow() != orig_window)
		glutSetWindow(orig_window);
	glutPostRedisplay();
}

void idle2() {
	if (glutGetWindow() != edited_window)
		glutSetWindow(edited_window);
	glutPostRedisplay();
}

bool isWithinBounds(int x, int y) {
	return x >= 0 && x < imgMod->GetWidth() && y >= 0 && y < imgMod->GetHeight();
}

void computeUIWidth() {
	int aux, vw;
	GLUI_Master.get_viewport_area(&aux, &aux, &vw, &aux);
	UI_width = img->GetWidth() - vw;
}

void reshape(int x, int y) {
	glutPostRedisplay();
}

void refresh(int window) {
	if (glutGetWindow() != window)
		glutSetWindow(window);
	glutPostRedisplay();
}

void loadImage() {
	img->Read(input);

	if (img->GetNumberOfChannels() == 3) {
		img->ConvertToGrayScale();
		img->SetNumberOfChannels(1);
	}

	imgMod->Copy(img);

	glutSetWindow(orig_window);
	glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

	glutSetWindow(edited_window);
	glutReshapeWindow(imgMod->GetWidth(), imgMod->GetHeight());
	refresh(edited_window);
}

static void display(void) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glRasterPos2i(-1, -1); // Fix raster position
	img->ViewImage();
	GLUI_Master.auto_set_viewport();

	glutSwapBuffers();
}

static void display2(void) {
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glRasterPos2i(-1, -1); // Fix raster position
	imgMod->ViewImage();
	GLUI_Master.auto_set_viewport();

	glutSwapBuffers();
}

typedef enum _mark
	: uByte {
		WHITE, GRAY, BLACK
} mark;

void applySegmentation() {

	for (const auto& seed : seedVector) {

		mark visited[imgMod->GetHeight()][imgMod->GetWidth()] { WHITE };

		std::vector<std::tuple<int, int>> positions;

		auto originX = seed.x;
		auto originY = seed.y;

		visited[originY][originX] = GRAY;

		positions.push_back(std::make_tuple(originX, originY));

		while (positions.size()) {

			auto lastX = std::get<0>(positions.back());
			auto lastY = std::get<1>(positions.back());

			auto hasNeighbor = false;

			for (int x = -1; x < 2; ++x) {

				for (int y = -1; y < 2; ++y) {

					// Root element
					if (!x && !y) {
						continue;
					}

					auto currentX = lastX + x;
					auto currentY = lastY + y;

					// Check image bounds
					if (currentX < 0 || currentY < 0
							|| currentX >= img->GetWidth()
							|| currentY >= img->GetHeight()) {
						continue;
					}

					if (visited[currentY][currentX] == WHITE
							&& (img->GetGrayValue(currentX, currentY)
									>= seed.min
									&& img->GetGrayValue(currentX, currentY)
											<= seed.max)) {
						positions.push_back(
								std::make_tuple(currentX, currentY));

						imgMod->SetGrayValue(currentX, currentY, 0xFF);

						visited[currentY][currentX] = GRAY;

						hasNeighbor = true;
					}
				}
			}

			if (!hasNeighbor) {
				visited[lastY][lastX] = BLACK;

				positions.pop_back();
			}
		}
	}
}

int calcMean(int floor, int roof) {
	int sum = 0;
	int mean;
	int histogram[255] = { 0 };

	pixel **mat;

	img->AllocatePixelMatrix(&mat, img->GetHeight(), img->GetWidth());
	img->GetDataAsMatrix(mat);

	for (int y = 0; y < img->GetHeight(); y++) {
		for (int x = 0; x < img->GetWidth(); x++) {
			int value = mat[y][x].value;

			if (value <= roof && value >= floor)
				sum += value;
		}
	}

	imgMod->SetDataAsMatrix(mat);
	imgMod->DeallocatePixelMatrix(&mat, imgMod->GetHeight(),
			imgMod->GetWidth());

	mean = sum / (img->GetWidth() * img->GetHeight());

	return mean;
}

void applyThreshold(int threshold) {
	/*
	 pixel **mat;

	 img->AllocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
	 img->GetDataAsMatrix(mat);

	 for (int y = 0; y < imgMod->GetHeight(); y++) {
	 for (int x = 0; x < imgMod->GetWidth(); x++) {
	 int value = mat[y][x].value >= threshold ? 255 : 0;

	 mat[y][x].value = value;
	 }
	 }

	 imgMod->SetDataAsMatrix(mat);
	 imgMod->DeallocatePixelMatrix(&mat, imgMod->GetHeight(),
	 imgMod->GetWidth());

	 refresh(edited_window);
	 */
	applySegmentation();

	refresh(edited_window);
}

void applyAutoThreshold() {
	int diff = 0;
	int i = 0;
	int threshold = calcMean(0, 255);

	do {
		int mean1 = calcMean(0, threshold);
		int mean2 = calcMean(threshold + 1, 255);

		int new_threshold = (mean2 + mean1) / 2;

		diff = std::abs(threshold - new_threshold);

		threshold = new_threshold;

	} while (diff > MIN_DIFF && i < MAX_ITER);

	spinner->set_int_val(threshold);

	applyThreshold(threshold);
}

void applyManualThreshold() {
	applyThreshold(spinner->get_int_val());
}

void control(int value) {
	switch (value) {
	case 1:
		// load
		loadImage();

		break;
	case 2:
		// save
		imgMod->Save(output);

		break;
	case 3:
		// radio
		if (option == 0) {
			// manual
			spinner->disable();
		} else if (option == 1) {
			// auto
			spinner->enable();
		}
		break;
	case 4:
		// spinner
		if (spinner->get_int_val() < 0)
			spinner->set_int_val(0);

		if (spinner->get_int_val() > 255)
			spinner->set_int_val(255);

		break;
	case 5:
		// apply
		if (option == 0) {
			applyAutoThreshold();
		} else if (option == 1) {
			applyManualThreshold();
		}

		break;
	case 6:
		// reset
		imgMod->Copy(img);
		refresh(edited_window);

		break;
	}

}

static void key(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
	case 'q':
		exit(0);
		break;
	}
	glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y) {
	/*if (option2 == 2) {
	 if (button == GLUT_LEFT_BUTTON) {
	 if (state == GLUT_DOWN) {
	 applyNotchFilter(
	 filterRadio,
	 x,
	 imgMod->GetHeight() - y);
	 }
	 }
	 }*/
}

void initGLUI() {
	// GLUI
	// Use the first line above to get the interface in the same window as the graphics
	// Use the second line (comment the first) to get the interface in a separated window
	//GLUI *glui = GLUI_Master.create_glui_subwindow(main_window,
	//GLUI_SUBWINDOW_RIGHT);
	GLUI* glui = GLUI_Master.create_glui("Control");
	GLUI_Master.set_glutReshapeFunc(reshape);
	GLUI_Panel* io_panel = glui->add_panel((char*) ("Input/Output"));
	GLUI_EditText *edit1, *edit2;
	edit1 = glui->add_edittext_to_panel(io_panel, (char*) (""),
	GLUI_EDITTEXT_TEXT, input);
	edit2 = glui->add_edittext_to_panel(io_panel, (char*) (""),
	GLUI_EDITTEXT_TEXT, output);
	edit1->set_w(185);
	edit2->set_w(185);
	glui->add_column_to_panel(io_panel, false);
	GLUI_Button* b1 = glui->add_button_to_panel(io_panel, (char*) ("Load"), 1,
			control);
	GLUI_Button* b2 = glui->add_button_to_panel(io_panel, (char*) ("Save"), 2,
			control);
	b1->set_w(50);
	b2->set_w(50);
	GLUI_Panel* filter_panel = glui->add_panel((char*) ("Manual Threshold"));
	radio2 = glui->add_radiogroup_to_panel(filter_panel, &option, 3, control);
	glui->add_radiobutton_to_group(radio2, (char*) ("Automatic"));
	glui->add_radiobutton_to_group(radio2, (char*) ("Manual"));
	spinner = glui->add_spinner_to_panel(filter_panel, (char*) ("Threshold"),
	GLUI_SPINNER_INT, &filterRadio, 4, control);
	spinner->disable();
	glui->add_button((char*) ("Apply"), 5, control);
	glui->add_button((char*) ("Reset"), 6, control);
	glui->add_button((char*) ("Quit"), 0, (GLUI_Update_CB) (exit));
	glui->set_main_gfx_window(orig_window);
	GLUI_Master.set_glutIdleFunc(idle);
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	strcpy(input, "figs/CT.lungs.png");
	strcpy(output, "figs/output.png");

	img = new PixelLab(input);
	imgMod = new PixelLab();
	imgMod->Copy(img);

	glutInitWindowSize(img->GetWidth(), img->GetHeight());
	glutInitWindowPosition(100, 100);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	orig_window = glutCreateWindow("Original Image");
	edited_window = glutCreateWindow("Edited Image");

	// original image window
	glutSetWindow(orig_window);
	glutKeyboardFunc(key);
	glutMouseFunc(mouse);
	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutPositionWindow(20, 20);

	// edited image window
	glutSetWindow(edited_window);
	glutKeyboardFunc(key);
	glutMouseFunc(mouse);
	glutIdleFunc(idle2);
	glutDisplayFunc(display2);
	glutReshapeFunc(reshape);
	glutPositionWindow(40 + img->GetWidth(), 20);

	// GLUI
	// Use the first line above to get the interface in the same window as the graphics
	// Use the second line (comment the first) to get the interface in a separated window
	//GLUI *glui = GLUI_Master.create_glui_subwindow(main_window,
	//GLUI_SUBWINDOW_RIGHT);
	initGLUI();
	computeUIWidth(); // Compute the size of the user interface
	glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

	glutMainLoop();

	return 1;
}
