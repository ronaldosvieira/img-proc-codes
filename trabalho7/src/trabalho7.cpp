#include "pixelLab.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <iostream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include "GL/glui.h"
#include "GL/glut.h"

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

void idle() {
	glutPostRedisplay();
}

bool isWithin(int radio, int x, int y) {
	// ideal
	return sqrt(pow(x, 2) + pow(y, 2)) <= radio;

	// butterworth
	// return 1 / pow(sqrt(pow(x, 2) + pow(y, 2)) / radio, 2 * 2);
}

bool isWithin(int radio, int x, int y, int centerx, int centery) {
	return sqrt(pow(x - centerx, 2) + pow(y - centery, 2)) <= radio;
}

bool isWithinBounds(int x, int y) {
	return x >= 0 && x < imgMod->GetWidth() &&
			y >= 0 && y < imgMod->GetHeight();
}

void computeUIWidth() {
	int aux, vw;
	GLUI_Master.get_viewport_area(&aux, &aux, &vw, &aux);
	UI_width = img->GetWidth() - vw;
}

void reshape(int x, int y) {
	glutPostRedisplay();
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

void applyThreshold() {
	// todo
}

void applyAutoThreshold() {
	// todo
}

void applyManualThreshold() {
	// todo
}

void control(int value) {
	switch (value) {
		case 1:
			// load
			img->Read(input);
			imgMod->Copy(img);
			glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

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
			applyAutoThreshold();

			break;
		case 6:
			// reset
			imgMod->Copy(img);
			spinner->set_int_val(128);

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

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	strcpy(input, "figs/lena.png");
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
	glutIdleFunc(idle);
	glutDisplayFunc(display2);
	glutReshapeFunc(reshape);
	glutPositionWindow(40 + img->GetWidth(), 20);

	// GLUI
	// Use the first line above to get the interface in the same window as the graphics
	// Use the second line (comment the first) to get the interface in a separated window
	//GLUI *glui = GLUI_Master.create_glui_subwindow(main_window,
	//GLUI_SUBWINDOW_RIGHT);
	GLUI *glui = GLUI_Master.create_glui("Control");

	GLUI_Master.set_glutReshapeFunc(reshape);

	GLUI_Panel *io_panel = glui->add_panel((char *) "Input/Output");
	GLUI_EditText *edit1, *edit2;
	edit1 = glui->add_edittext_to_panel(io_panel, (char *) "",
	GLUI_EDITTEXT_TEXT, input);
	edit2 = glui->add_edittext_to_panel(io_panel, (char *) "",
	GLUI_EDITTEXT_TEXT, output);
	edit1->set_w(185);
	edit2->set_w(185);
	glui->add_column_to_panel(io_panel, false);
	GLUI_Button *b1 = glui->add_button_to_panel(io_panel, (char*) "Load", 1,
			control);
	GLUI_Button *b2 = glui->add_button_to_panel(io_panel, (char*) "Save", 2,
			control);
	b1->set_w(50);
	b2->set_w(50);

	GLUI_Panel *filter_panel = glui->add_panel((char *) "Manual Threshold");

	radio2 = glui->add_radiogroup_to_panel(filter_panel, &option, 3, control);
	glui->add_radiobutton_to_group(radio2, (char *) "Automatic");
	glui->add_radiobutton_to_group(radio2, (char *) "Manual");

	spinner = glui->add_spinner_to_panel( filter_panel, (char*)"Threshold",
			GLUI_SPINNER_INT, &filterRadio, 4, control);
	spinner->disable();

	glui->add_button((char *) "Apply", 5, control);

	glui->add_button((char *) "Reset", 6, control);
	glui->add_button((char *) "Quit", 0, (GLUI_Update_CB) exit);

	glui->set_main_gfx_window(orig_window);
	GLUI_Master.set_glutIdleFunc(idle);
	computeUIWidth(); // Compute the size of the user interface
	glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

	glutMainLoop();

	return 1;
}
