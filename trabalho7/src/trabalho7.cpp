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

// Image Objects
PixelLab *img = NULL;
PixelLab *imgMod = NULL;

GLUI_RadioGroup *radio, *radio2;
GLUI_RadioButton *buttons[3];
GLUI_Button *transformButton;
GLUI_Spinner *noiseSpinner, *spinner;

int main_window;
int UI_width = 0;
int option = 0;
int option2 = 0;
int filterRadio = 15;
float noiseProb = 0.05;
int prevBrightness = 0;

char input[512];
char output[512];

bool isTransformed = false;

void idle() {
	if (glutGetWindow() != main_window)
		glutSetWindow(main_window);
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

void updateUI(bool newIsTransformed) {
	if (newIsTransformed) {
		transformButton->disable();
		noiseSpinner->disable();
		spinner->enable();

		isTransformed = true;
	} else {
		transformButton->enable();
		noiseSpinner->enable();
		spinner->disable();

		isTransformed = false;
	}
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
	imgMod->ViewImage();
	GLUI_Master.auto_set_viewport();

	glutSwapBuffers();
}

void control(int value) {
	switch (value) {
		case 1:
			img->Read(input);
			imgMod->Copy(img);
			glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

			updateUI(false);
			break;
		case 2:
			imgMod->Save(output);
			break;
		case 3:
			//applySaltAndPepper();
			break;
		case 4:
			/*imgMod->Copy(img);

			updateUI(false);
			radio2->set_int_val(0);*/

			break;
		case 5:
			/*if (option2 == 2) {
				if (!isTransformed) {
					//shiftFFT();
					//applyFFT();

					updateUI(true);
				}
			} else {
				if (isTransformed) {
					//applyIFFT();
					//shiftFFT();

					updateUI(false);
				}
			}*/
			break;
		case 6:
			/*if (option2 == 1) {
				//applyMedianFilter();
			} else if (option2 == 2) {
				radio2->set_int_val(0);
				control(5);
			}*/

			break;
		case 7:
			//if (spinner->get_int_val() < 0) spinner->set_int_val(0);
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
	std::srand(std::time(NULL));
	glutInit(&argc, argv);
	strcpy(input, "figs/lena.png");
	strcpy(output, "figs/output.png");

	img = new PixelLab(input);
	imgMod = new PixelLab();
	imgMod->Copy(img);

	glutInitWindowSize(img->GetWidth(), img->GetHeight());
	glutInitWindowPosition(100, 100);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	main_window = glutCreateWindow("Image Segmentation");

	glutKeyboardFunc(key);
	glutMouseFunc(mouse);
	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

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

	GLUI_Panel *channel_panel = glui->add_panel((char *) "Noise");

	transformButton =
			glui->add_button_to_panel(channel_panel, (char *) "Add Salt & Pepper", 3, control);
	noiseSpinner = glui->add_spinner_to_panel( channel_panel, (char*)"Probability", GLUI_SPINNER_FLOAT, &noiseProb, 8, control);
	noiseSpinner->set_float_limits(0.0, 1.0);
	noiseSpinner->set_speed(0.1);

	GLUI_Panel *filter_panel = glui->add_panel((char *) "Noise Removal Strategy");
	radio2 = glui->add_radiogroup_to_panel(filter_panel, &option2, 5, control);
	glui->add_radiobutton_to_group(radio2, (char *) "None");
	glui->add_radiobutton_to_group(radio2, (char *) "Median Filter");
	glui->add_radiobutton_to_group(radio2, (char *) "Notch Filter");

	spinner = glui->add_spinner_to_panel( filter_panel, (char*)"Radio", GLUI_SPINNER_INT, &filterRadio, 7, control);
	spinner->disable();

	glui->add_button((char *) "Apply", 6, control);

	glui->add_button((char *) "Reset", 4, control);
	glui->add_button((char *) "Quit", 0, (GLUI_Update_CB) exit);

	glui->set_main_gfx_window(main_window);
	GLUI_Master.set_glutIdleFunc(idle);
	computeUIWidth(); // Compute the size of the user interface
	glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

	glutMainLoop();

	return 1;
}
