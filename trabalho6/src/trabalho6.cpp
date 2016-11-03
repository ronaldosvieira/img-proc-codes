#include "pixelLab.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <iostream>
#include <cmath>
#include "GL/glui.h"
#include "GL/glut.h"
#include "fft2d.c"

// Image Objects
PixelLab *img = NULL;
PixelLab *imgMod = NULL;

GLUI_RadioGroup *radio, *radio2;
GLUI_RadioButton *buttons[3];
GLUI_Button *transformButton;
GLUI_Spinner *spinner;

int main_window;
int UI_width = 0;
int option = 0;
int option2 = 0;
int filterRadio = 0;
int prevBrightness = 0;

char input[512];
char output[512];

COMPLEX **m;

bool isTransformed = false;
bool isFiltered = false;

void idle() {
	if (glutGetWindow() != main_window)
		glutSetWindow(main_window);
	glutPostRedisplay();
}

pixel** slice(pixel **m, int sx, int sy, int width, int height) {
	pixel **s = new pixel*[height];

	for (int i = 0; i < height; ++i) {
		s[i] = new pixel[width];
	}

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			s[x][y].value = m[x + sx][y + sy].value;
		}
	}

	return s;
}

double isWithin(int radio, int x, int y) {
	// ideal
	return sqrt(pow(x, 2) + pow(y, 2)) <= radio;

	// butterworth
	// return 1 / pow(sqrt(pow(x, 2) + pow(y, 2)) / radio, 2 * 2);
}

void initMatrix() {
	m = (COMPLEX**) malloc(sizeof(COMPLEX*) * imgMod->GetWidth());

	for (int i = 0; i < imgMod->GetWidth(); i++) {
		m[i] = (COMPLEX*) malloc(sizeof(COMPLEX) * imgMod->GetHeight());
	}
}

void updateMatrix() {
	for (int y = 0; y < imgMod->GetHeight(); y++) {
		for (int x = 0; x < imgMod->GetWidth(); x++) {
			m[x][y].real = imgMod->GetGrayValue(x, y);
			m[x][y].imag = 0;
		}
	}
}

void updateUI(bool newIsTransformed) {
	if (newIsTransformed) {
		transformButton->set_name("Apply IFFT");
		radio2->enable();
		spinner->enable();

		isTransformed = true;
	} else {
		transformButton->set_name("Apply FFT");
		radio2->disable();
		spinner->disable();

		isTransformed = false;
		isFiltered = false;
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

void applyFFT() {
	if (img) {
		if (imgMod->GetNumberOfChannels() == 3) imgMod->ConvertToGrayScale();

		FFT2D(m, imgMod->GetWidth(), imgMod->GetHeight(), 1);

		for (int y = 0; y < imgMod->GetHeight(); y++) {
			for (int x = 0; x < imgMod->GetWidth(); x++) {
				double value = sqrt(pow(m[x][y].real, 2) + pow(m[x][y].imag, 2)) * 200;

				// adiciona um valor arbitrário
				if (value > 0) value += 55;

				if (value > 255) value = 255;
				if (value < 0) value = 0;

				imgMod->SetGrayValue(x, y, value);
			}
		}
	}
}

void applyIFFT() {
	if (img) {
		if (imgMod->GetNumberOfChannels() == 3) imgMod->ConvertToGrayScale();

		FFT2D(m, imgMod->GetWidth(), imgMod->GetHeight(), -1);

		for (int y = 0; y < imgMod->GetHeight(); y++) {
			for (int x = 0; x < imgMod->GetWidth(); x++) {
				double value = sqrt(pow(m[x][y].real, 2) + pow(m[x][y].imag, 2));

				if (value > 255) value = 255;
				if (value < 0) value = 0;

				imgMod->SetGrayValue(x, y, value);
			}
		}
	}
}

void shiftFFT() {
	pixel **m;

	imgMod->AllocatePixelMatrix(&m, imgMod->GetWidth(), imgMod->GetHeight());
	imgMod->GetDataAsMatrix(m);

	if (imgMod->GetNumberOfChannels() == 1) {
		for (int y = 0; y < img->GetHeight(); y++) {
			for (int x = 0; x < img->GetWidth(); x++) {
				m[y][x].value *= pow(-1, x + y);
			}
		}
	} else {
		for (int y = 0; y < img->GetHeight(); y++) {
			for (int x = 0; x < img->GetWidth(); x++) {
				m[y][x].R *= pow(-1, x + y);
				m[y][x].G *= pow(-1, x + y);
				m[y][x].B *= pow(-1, x + y);
			}
		}
	}

 	imgMod->SetDataAsMatrix(m);
	imgMod->DeallocatePixelMatrix(&m, imgMod->GetHeight(), imgMod->GetWidth());

	updateMatrix();
}

void applyLowPass(int radio) {
	pixel **mat;

	imgMod->AllocatePixelMatrix(&mat, imgMod->GetWidth(), imgMod->GetHeight());
	imgMod->GetDataAsMatrix(mat);

	int centerx = imgMod->GetWidth() / 2;
	int centery = imgMod->GetHeight() / 2;

	if (imgMod->GetNumberOfChannels() == 1) {
		for (int y = 0; y < img->GetHeight(); y++) {
			for (int x = 0; x < img->GetWidth(); x++) {
				if (!isWithin(radio, x - centerx, y - centery)) {
					mat[y][x].value = 0;
					m[x][y].real = 0;
					m[x][y].imag = 0;
				}
				/*double c = isWithin(radio, x - centerx, y - centery);

				mat[y][x].value *= c > 1? 1 : c;
				m[x][y].real *= c > 1? 1 : c;
				m[x][y].imag *= c > 1? 1 : c;*/
			}
		}
	} else {
		for (int y = 0; y < img->GetHeight(); y++) {
			for (int x = 0; x < img->GetWidth(); x++) {
				if (!isWithin(radio, x - centerx, y - centery)) {
					mat[y][x].R = 0;
					mat[y][x].G = 0;
					mat[y][x].B = 0;
					m[x][y].real = 0;
					m[x][y].imag = 0;
				}
			}
		}
	}

	imgMod->SetDataAsMatrix(mat);
	imgMod->DeallocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
}

void applyHighPass(int radio) {
	pixel **mat;

	imgMod->AllocatePixelMatrix(&mat, imgMod->GetWidth(), imgMod->GetHeight());
	imgMod->GetDataAsMatrix(mat);

	int centerx = imgMod->GetWidth() / 2;
	int centery = imgMod->GetHeight() / 2;

	if (imgMod->GetNumberOfChannels() == 1) {
		for (int y = 0; y < img->GetHeight(); y++) {
			for (int x = 0; x < img->GetWidth(); x++) {
				if (isWithin(radio, x - centerx, y - centery)) {
					mat[y][x].value = 0;
					m[x][y].real = 0;
					m[x][y].imag = 0;
				}
				/*double c = isWithin(radio, x - centerx, y - centery);

				mat[y][x].value *= c > 1? 1 : c;
				m[x][y].real *= c > 1? 1 : c;
				m[x][y].imag *= c > 1? 1 : c;*/
			}
		}
	} else {
		for (int y = 0; y < img->GetHeight(); y++) {
			for (int x = 0; x < img->GetWidth(); x++) {
				if (isWithin(radio, x - centerx, y - centery)) {
					mat[y][x].R = 0;
					mat[y][x].G = 0;
					mat[y][x].B = 0;
					m[x][y].real = 0;
					m[x][y].imag = 0;
				}
			}
		}
	}

	imgMod->SetDataAsMatrix(mat);
	imgMod->DeallocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
}

void control(int value) {
	switch (value) {
	case 1:
		img->Read(input);
		imgMod->Copy(img);
		updateMatrix();
		glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

		updateUI(false);
		isTransformed = false;
		break;
	case 2:
		imgMod->Save(output);
		break;
	case 3:
		if (!isTransformed) {
			shiftFFT();
			applyFFT();

			updateUI(true);
		} else {
			applyIFFT();
			if (!isFiltered) shiftFFT();

			updateUI(false);
		}
		break;
	case 4:
		imgMod->Copy(img);
		updateMatrix();

		updateUI(false);
		isTransformed = false;
		break;
	case 6:
		if (option2 == 0) {
			applyLowPass(filterRadio);
			isFiltered = true;
		} else if (option2 == 1) {
			applyHighPass(filterRadio);
			isFiltered = true;
		}
		break;
	case 7:
		if (spinner->get_int_val() < 0) spinner->set_int_val(0);
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

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	strcpy(input, "figs/lenaGray.png");
	strcpy(output, "figs/output.png");

	img = new PixelLab(input);
	imgMod = new PixelLab();
	imgMod->Copy(img);

	initMatrix();
	updateMatrix();

	glutInitWindowSize(img->GetWidth(), img->GetHeight());
	glutInitWindowPosition(100, 100);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	main_window = glutCreateWindow("Image Filtering in the Frequency Domain");

	glutKeyboardFunc(key);
	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	// GLUI
	// Use the first line above to get the interface in the same window as the graphics
	// Use the second line (comment the first) to get the interface in a separeted window
	//GLUI *glui = GLUI_Master.create_glui_subwindow(main_window,
	//GLUI_SUBWINDOW_RIGHT);
	GLUI *glui = GLUI_Master.create_glui( "" );

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

	GLUI_Panel *channel_panel = glui->add_panel((char *) " Fourier Transform");

	transformButton = glui->add_button_to_panel(channel_panel, (char *) "Apply FFT", 3, control);

	GLUI_Panel *filter_panel = glui->add_panel((char *) "Filter");
	radio2 = glui->add_radiogroup_to_panel(filter_panel, &option2, 5, (GLUI_Update_CB) NULL);
	glui->add_radiobutton_to_group(radio2, (char *) "Low-pass                             ");
	glui->add_radiobutton_to_group(radio2, (char *) "High-pass");
	radio2->disable();

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

	free(m);

	return 1;
}
