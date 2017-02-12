#include "pixelLab.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <iostream>
#include <vector>
#include <tuple>
#include <utility>
#include <cmath>
#include <sstream>
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

//GLUI_RadioGroup *radio, *radio2;
//GLUI_RadioGroup *radio;
//GLUI_RadioButton *buttons[3];
//GLUI_Button *autoThresholdButton, *manualThresholdButton;
//GLUI_Spinner *noiseSpinner, *spinner;

GLUI_Listbox* seed_list = nullptr;

int orig_window, edited_window;
int UI_width = 0;
int option = 0;
int option2 = 0;
int filterRadio = 128;
float noiseProb = 0.05;
int prevBrightness = 0;

bool pickingPoint = false;

char input[512];
char output[512];

bool isTransformed = false;

int selected_seed = -1;

int lineX = 0;
int lineY = 0;

float lineX_norm = 0;
float lineY_norm = 0;

// Seed edit values
int edit_value_x, edit_value_y;
int edit_value_r, edit_value_g, edit_value_b;
int edit_value_min, edit_value_max;

// Seed edit values
GLUI_Spinner *edit_seed_x, *edit_seed_y;
GLUI_Spinner *edit_seed_r, *edit_seed_g, *edit_seed_b;
GLUI_Spinner *edit_seed_min, *edit_seed_max;

// Seed set values button
GLUI_Button* button_set_values;

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

void drawLines(float x, float y);

std::vector<seed> seed_vector;

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

	if (pickingPoint) {
		drawLines(lineX_norm, lineY_norm);

		edit_seed_x->set_int_val(lineX);
		edit_seed_y->set_int_val(lineY);
	}

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

	pixel **mat;

	img->AllocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
	img->GetDataAsMatrix(mat);

	imgMod->SetDataAsMatrix(mat);

	for (const auto& seed : seed_vector) {

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

						imgMod->SetRGB(currentX, currentY, seed.c.r, seed.c.g,
								seed.c.b);

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

	imgMod->DeallocatePixelMatrix(&mat, imgMod->GetHeight(),
			imgMod->GetWidth());

	refresh(edited_window);
}

void drawLines(float x, float y) {
	glColor3f(1.0, 0, 0);
	glBegin(GL_LINES);

	glVertex2f(-1, lineY_norm);
	glVertex2f(1, lineY_norm);

	glVertex2f(lineX_norm, -1);
	glVertex2f(lineX_norm, 1);

	glEnd();
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

		break;
	case 4:
		// spinner

		break;
	case 5:
		applySegmentation();

		break;
	case 6: {
		// reset
		button_set_values->disable();

		edit_seed_x->set_int_val(0);
		edit_seed_y->set_int_val(0);

		edit_seed_r->set_int_val(0);
		edit_seed_g->set_int_val(0);
		edit_seed_b->set_int_val(0);

		edit_seed_min->set_int_val(0);
		edit_seed_max->set_int_val(0);

		for (int i = 0; i < seed_vector.size(); ++i) {
			seed_list->delete_item(i);
		}

		seed_vector.clear();

		imgMod->Copy(img);
		refresh(edited_window);

		break;
	}

	case 7: {

		// List of seeds
		if (selected_seed < 0) {
			button_set_values->disable();

			edit_seed_x->set_int_val(0);
			edit_seed_y->set_int_val(0);

			edit_seed_r->set_int_val(0);
			edit_seed_g->set_int_val(0);
			edit_seed_b->set_int_val(0);

			edit_seed_min->set_int_val(0);
			edit_seed_max->set_int_val(0);
		} else {
			button_set_values->enable();

			const auto& seed = seed_vector.at(selected_seed);

			edit_seed_x->set_int_val(seed.x);
			edit_seed_y->set_int_val(seed.y);

			edit_seed_r->set_int_val((int) seed.c.r);
			edit_seed_g->set_int_val((int) seed.c.g);
			edit_seed_b->set_int_val((int) seed.c.b);

			edit_seed_min->set_int_val((int) seed.min);
			edit_seed_max->set_int_val((int) seed.max);
		}

		break;
	}

	case 8: {
		// Add seed button
		seed new_seed { edit_value_x, edit_value_y, (uByte) edit_value_min,
				(uByte) edit_value_max, { (uByte) edit_value_r,
						(uByte) edit_value_g, (uByte) edit_value_b } };

		seed_vector.push_back(new_seed);

		std::stringstream ss;
		ss << "Seed ";
		ss << seed_vector.size();

		seed_list->add_item(seed_vector.size(), ss.str().c_str());

		break;
	}

	case 9: {
		// Set seed values button
		if (selected_seed >= 0) {
			auto& seed = seed_vector.at(selected_seed);

			seed.x = edit_value_x;
			seed.y = edit_value_y;

			seed.c.r = edit_value_r;
			seed.c.g = edit_value_g;
			seed.c.b = edit_value_b;

			seed.min = edit_value_min;
			seed.max = edit_value_max;
		}

		break;
	}

	case 10: {
		// Pick position of seed
		pickingPoint = true;
	}
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

	if (pickingPoint && button == GLUT_LEFT_BUTTON) {
		pickingPoint = false;
	}
}

#define NORM_Y(f) (((float) f) /img->GetHeight())
#define NORM_X(f) (((float) f) /img->GetWidth())

static void mouseMotion(int x, int y) {
	lineX = x;
	lineY = img->GetHeight() - y;

	lineY_norm = NORM_Y((img->GetHeight()- y) - img->GetHeight() / 2) * 2;
	lineX_norm = NORM_X(x - img->GetWidth() / 2) * 2;
}

#undef NORM_Y
#undef NORM_X

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

	// Seeds panel
	GLUI_Panel* seed_panel = glui->add_panel((char*) ("Seeds"));

	// List of seeds
	seed_list = glui->add_listbox_to_panel(seed_panel, "Seed: ", &selected_seed,
			7, control);
	seed_list->set_w(185);

	seed_list->add_item(-1, "(None)");

	// Seed position
	edit_seed_x = glui->add_spinner_to_panel(seed_panel, (char*) ("X"),
	GLUI_SPINNER_INT, &edit_value_x);
	edit_seed_x->set_w(185);
	edit_seed_x->set_int_limits(0, img->GetWidth());

	edit_seed_y = glui->add_spinner_to_panel(seed_panel, (char*) ("Y"),
	GLUI_EDITTEXT_INT, &edit_value_y);
	edit_seed_y->set_w(185);
	edit_seed_y->set_int_limits(0, img->GetHeight());

	// Seed pick position
	GLUI_Button* button_picxy_seed = glui->add_button_to_panel(seed_panel,
			(char*) ("Pick position"), 10, control);

	// Seed color
	edit_seed_r = glui->add_spinner_to_panel(seed_panel, (char*) ("R"),
	GLUI_EDITTEXT_INT, &edit_value_r);
	edit_seed_r->set_w(185);
	edit_seed_r->set_int_limits(0, 255);

	edit_seed_g = glui->add_spinner_to_panel(seed_panel, (char*) ("G"),
	GLUI_EDITTEXT_INT, &edit_value_g);
	edit_seed_g->set_w(185);
	edit_seed_g->set_int_limits(0, 255);

	edit_seed_b = glui->add_spinner_to_panel(seed_panel, (char*) ("B"),
	GLUI_EDITTEXT_INT, &edit_value_b);
	edit_seed_b->set_w(185);
	edit_seed_b->set_int_limits(0, 255);

	// Seed threshold
	edit_seed_min = glui->add_spinner_to_panel(seed_panel, (char*) ("Min"),
	GLUI_EDITTEXT_INT, &edit_value_min);
	edit_seed_min->set_w(185);
	edit_seed_min->set_int_limits(0, 255);

	edit_seed_max = glui->add_spinner_to_panel(seed_panel, (char*) ("Max"),
	GLUI_EDITTEXT_INT, &edit_value_max);
	edit_seed_max->set_w(185);
	edit_seed_max->set_int_limits(0, 255);

	// Save button
	button_set_values = glui->add_button_to_panel(seed_panel,
			(char*) ("Set values"), 9, control);
	button_set_values->disable();

	// Add seed button
	GLUI_Button* button_add_seed = glui->add_button_to_panel(seed_panel,
			(char*) ("Add new seed"), 8, control);

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
	glutPassiveMotionFunc(mouseMotion);
	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutPositionWindow(20, 20);

	// edited image window
	glutSetWindow(edited_window);
	glutKeyboardFunc(key);
//	glutMouseFunc(mouseOrigin);
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
