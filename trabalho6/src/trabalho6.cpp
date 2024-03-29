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
#include "fft2d.c"

#define FOURIER_OFFSET 55;

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

COMPLEX **m, **mr, **mg, **mb;

bool isTransformed = false;

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

void initMatrix() {
	m = (COMPLEX**) malloc(sizeof(COMPLEX*) * imgMod->GetWidth());
	mr = (COMPLEX**) malloc(sizeof(COMPLEX*) * imgMod->GetWidth());
	mg = (COMPLEX**) malloc(sizeof(COMPLEX*) * imgMod->GetWidth());
	mb = (COMPLEX**) malloc(sizeof(COMPLEX*) * imgMod->GetWidth());

	for (int i = 0; i < imgMod->GetWidth(); i++) {
		m[i] = (COMPLEX*) malloc(sizeof(COMPLEX) * imgMod->GetHeight());
		mr[i] = (COMPLEX*) malloc(sizeof(COMPLEX) * imgMod->GetHeight());
		mg[i] = (COMPLEX*) malloc(sizeof(COMPLEX) * imgMod->GetHeight());
		mb[i] = (COMPLEX*) malloc(sizeof(COMPLEX) * imgMod->GetHeight());
	}
}

void updateMatrix() {
	if (imgMod->GetNumberOfChannels() == 1) {
		for (int y = 0; y < imgMod->GetHeight(); y++) {
			for (int x = 0; x < imgMod->GetWidth(); x++) {
				m[x][y].real = imgMod->GetGrayValue(x, y);
				m[x][y].imag = 0;
			}
		}
	} else {
		for (int y = 0; y < imgMod->GetHeight(); y++) {
			for (int x = 0; x < imgMod->GetWidth(); x++) {
				mr[x][y].real = imgMod->GetR(x, y);
				mr[x][y].imag = 0;
				mg[x][y].real = imgMod->GetG(x, y);
				mg[x][y].imag = 0;
				mb[x][y].real = imgMod->GetB(x, y);
				mb[x][y].imag = 0;
			}
		}
	}
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

void applyFFT() {
	if (img) {
		if (imgMod->GetNumberOfChannels() == 1) {
			FFT2D(m, imgMod->GetWidth(), imgMod->GetHeight(), 1);

			for (int y = 0; y < imgMod->GetHeight(); y++) {
				for (int x = 0; x < imgMod->GetWidth(); x++) {
					double value = sqrt(pow(m[x][y].real, 2) + pow(m[x][y].imag, 2)) * 200;

					// adiciona um valor arbitrário
					if (value > 0) value += FOURIER_OFFSET;

					if (value > 255) value = 255;
					if (value < 0) value = 0;

					imgMod->SetGrayValue(x, y, value);
				}
			}
		} else {
			FFT2D(mr, imgMod->GetWidth(), imgMod->GetHeight(), 1);
			FFT2D(mg, imgMod->GetWidth(), imgMod->GetHeight(), 1);
			FFT2D(mb, imgMod->GetWidth(), imgMod->GetHeight(), 1);

			for (int y = 0; y < imgMod->GetHeight(); y++) {
				for (int x = 0; x < imgMod->GetWidth(); x++) {
					double valueR = sqrt(pow(mr[x][y].real, 2) + pow(mr[x][y].imag, 2)) * 200;
					double valueG = sqrt(pow(mg[x][y].real, 2) + pow(mg[x][y].imag, 2)) * 200;
					double valueB = sqrt(pow(mb[x][y].real, 2) + pow(mb[x][y].imag, 2)) * 200;

					// adiciona um valor arbitrário
					if (valueR > 0) valueR += FOURIER_OFFSET;
					if (valueG > 0) valueG += FOURIER_OFFSET;
					if (valueB > 0) valueB += FOURIER_OFFSET;

					if (valueR > 255) valueR = 255;
					if (valueG > 255) valueG = 255;
					if (valueB > 255) valueB = 255;

					if (valueR < 0) valueR = 0;
					if (valueG < 0) valueG = 0;
					if (valueB < 0) valueB = 0;

					imgMod->SetRGB(x, y, valueR, valueG, valueB);
				}
			}
		}
	}
}

void applyIFFT() {
	if (img) {
		if (imgMod->GetNumberOfChannels() == 1) {
			FFT2D(m, imgMod->GetWidth(), imgMod->GetHeight(), -1);

			for (int y = 0; y < imgMod->GetHeight(); y++) {
				for (int x = 0; x < imgMod->GetWidth(); x++) {
					double value = sqrt(pow(m[x][y].real, 2) + pow(m[x][y].imag, 2));

					if (value > 255) value = 255;
					if (value < 0) value = 0;

					imgMod->SetGrayValue(x, y, value);
				}
			}
		} else {
			FFT2D(mr, imgMod->GetWidth(), imgMod->GetHeight(), -1);
			FFT2D(mg, imgMod->GetWidth(), imgMod->GetHeight(), -1);
			FFT2D(mb, imgMod->GetWidth(), imgMod->GetHeight(), -1);

			for (int y = 0; y < imgMod->GetHeight(); y++) {
				for (int x = 0; x < imgMod->GetWidth(); x++) {
					double valueR = sqrt(pow(mr[x][y].real, 2) + pow(mr[x][y].imag, 2));
					double valueG = sqrt(pow(mg[x][y].real, 2) + pow(mg[x][y].imag, 2));
					double valueB = sqrt(pow(mb[x][y].real, 2) + pow(mb[x][y].imag, 2));

					if (valueR > 255) valueR = 255;
					if (valueG > 255) valueG = 255;
					if (valueB > 255) valueB = 255;

					if (valueR < 0) valueR = 0;
					if (valueG < 0) valueG = 0;
					if (valueB < 0) valueB = 0;

					imgMod->SetRGB(x, y, valueR, valueG, valueB);
				}
			}
		}
	}
}

void shiftFFT() {
	pixel **m;

	imgMod->AllocatePixelMatrix(&m, imgMod->GetHeight(), imgMod->GetWidth());
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

void applySaltAndPepper() {
	if (img) {
		pixel **mat;
		double num;
		int channels = imgMod->GetNumberOfChannels();

		imgMod->AllocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
		imgMod->GetDataAsMatrix(mat);

		for (int y = 0; y < imgMod->GetHeight(); y++) {
			for (int x = 0; x < imgMod->GetWidth(); x++) {
				num = (double) std::rand() / RAND_MAX;

				if (num < noiseProb) {
					num = (double) std::rand() / RAND_MAX;

					if (channels == 1) {
						mat[y][x].value = num < 0.5? 0 : 255;
					} else {
						mat[y][x].R = num < 0.5? 0 : 255;
						mat[y][x].G = num < 0.5? 0 : 255;
						mat[y][x].B = num < 0.5? 0 : 255;
					}
				}
			}
		}

		imgMod->SetDataAsMatrix(mat);
		imgMod->DeallocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
	}
}

void applyMedianFilter() {
	if (img) {
		pixel **mat;
		int i, j;

		imgMod->AllocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
		imgMod->GetDataAsMatrix(mat);

		if (imgMod->GetNumberOfChannels() == 1) {
			int nbh[9] = {0};

			for (int y = 0; y < imgMod->GetHeight(); y++) {
				for (int x = 0; x < imgMod->GetWidth(); x++) {

					for (int dy = 0; dy < 3; dy++) {
						for (int dx = 0; dx < 3; dx++) {
							i = x + dx - 1;
							j = y + dy - 1;

							if (i >= 0 && i < imgMod->GetWidth() &&
								j >= 0 && j < imgMod->GetHeight()) {
								nbh[dx + dy * 3] = mat[y + dy - 1][x + dx - 1].value;
							}
						}
					}

					std::sort(nbh, nbh + 9);
					mat[y][x].value = nbh[4];
				}
			}
		} else {
			int nbhr[9] = {0};
			int nbhg[9] = {0};
			int nbhb[9] = {0};

			for (int y = 0; y < imgMod->GetHeight(); y++) {
				for (int x = 0; x < imgMod->GetWidth(); x++) {

					for (int dy = 0; dy < 3; dy++) {
						for (int dx = 0; dx < 3; dx++) {
							i = x + dx - 1;
							j = y + dy - 1;

							if (i >= 0 && i < imgMod->GetWidth() &&
								j >= 0 && j < imgMod->GetHeight()) {
								nbhr[dx + dy * 3] = mat[y + dy - 1][x + dx - 1].R;
								nbhg[dx + dy * 3] = mat[y + dy - 1][x + dx - 1].G;
								nbhb[dx + dy * 3] = mat[y + dy - 1][x + dx - 1].G;
							}
						}
					}

					std::sort(nbhr, nbhr + 9);
					std::sort(nbhg, nbhg + 9);
					std::sort(nbhb, nbhb + 9);

					mat[y][x].R = nbhr[4];
					mat[y][x].G = nbhg[4];
					mat[y][x].B = nbhb[4];
				}
			}
		}

		imgMod->SetDataAsMatrix(mat);
		imgMod->DeallocatePixelMatrix(&mat, imgMod->GetHeight(), imgMod->GetWidth());
	}
}

void applyNotchFilter(int radio, int centerx, int centery) {
	pixel **mat;

	imgMod->AllocatePixelMatrix(&mat, imgMod->GetWidth(), imgMod->GetHeight());
	imgMod->GetDataAsMatrix(mat);

	if (imgMod->GetNumberOfChannels() == 1) {
		for (int j = centery - radio; j < centery + radio; j++) {
			for (int i = centerx - radio; i < centerx + radio; i++) {
				if (isWithin(radio, i, j, centerx, centery)
						&& isWithinBounds(i, j)) {
					mat[j][i].value = FOURIER_OFFSET;
					m[i][j].real = 0;
					m[i][j].imag = 0;
				}
			}
		}
	} else {
		for (int j = centery - radio; j < centery + radio; j++) {
			for (int i = centerx - radio; i < centerx + radio; i++) {
				if (isWithin(radio, i, j, centerx, centery)
						&& isWithinBounds(i, j)) {
					mat[j][i].R = FOURIER_OFFSET;
					mat[j][i].G = FOURIER_OFFSET;
					mat[j][i].B = FOURIER_OFFSET;

					mr[i][j].real = 0; mr[i][j].imag = 0;
					mg[i][j].real = 0; mg[i][j].imag = 0;
					mb[i][j].real = 0; mb[i][j].imag = 0;
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
		glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

		updateUI(false);
		isTransformed = false;
		break;
	case 2:
		imgMod->Save(output);
		break;
	case 3:
		applySaltAndPepper();
		break;
	case 4:
		imgMod->Copy(img);

		updateUI(false);
		radio2->set_int_val(0);

		isTransformed = false;
		break;
	case 5:
		if (option2 == 2) {
			if (!isTransformed) {
				shiftFFT();
				applyFFT();

				updateUI(true);
			}
		} else {
			if (isTransformed) {
				applyIFFT();
				shiftFFT();

				updateUI(false);
			}
		}
		break;
	case 6:
		if (option2 == 1) {
			applyMedianFilter();
		} else if (option2 == 2) {
			radio2->set_int_val(0);
			control(5);
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

static void mouse(int button, int state, int x, int y) {
	if (option2 == 2) {
		if (button == GLUT_LEFT_BUTTON) {
			if (state == GLUT_DOWN) {
				applyNotchFilter(
						filterRadio,
						x,
						imgMod->GetHeight() - y);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	std::srand(std::time(NULL));
	glutInit(&argc, argv);
	strcpy(input, "figs/lena.png");
	strcpy(output, "figs/output.png");

	img = new PixelLab(input);
	imgMod = new PixelLab();
	imgMod->Copy(img);

	initMatrix();
	updateMatrix();

	isTransformed = false;

	glutInitWindowSize(img->GetWidth(), img->GetHeight());
	glutInitWindowPosition(100, 100);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	main_window = glutCreateWindow("Image Restoration");

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
