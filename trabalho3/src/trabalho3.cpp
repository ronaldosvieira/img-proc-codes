#include "pixelLab.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <math.h>
#include "GL/glui.h"
#include "GL/glut.h"

// Image Objects
PixelLab *img    = NULL;
PixelLab *imgMod = NULL;

GLUI_RadioGroup *radio;
int main_window;
int UI_width  = 0;
int option = 0;
int brightness = 0;
int prevBrightness = 0;

char input[512];
char output[512];

void idle()
{
   if ( glutGetWindow() != main_window)
      glutSetWindow(main_window);
   glutPostRedisplay();
}

void computeUIWidth()
{
   int aux, vw;
   GLUI_Master.get_viewport_area(&aux, &aux, &vw, &aux);
   UI_width = img->GetWidth()-vw;
}

void reshape( int x, int y )
{
  glutPostRedisplay();
}

static void display(void)
{
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glRasterPos2i(-1, -1); // Fix raster position
   imgMod->ViewImage();
   GLUI_Master.auto_set_viewport();

   glutSwapBuffers();
}

void modifyImage(char filter, int size) {
   if (img) {
	   //pixel **m;

	   imgMod->Copy(img);

	   if (size <= 0) return;
	   if (size % 2 == 0) size--;

	   printf("%d\n", size);

	   //imgMod->AllocatePixelMatrix(&m, imgMod->GetHeight(), imgMod->GetWidth());
	   //imgMod->GetDataAsMatrix(m);

      for (int y = 0; y < img->GetHeight(); y++) {
         for (int x = 0; x < img->GetWidth(); x++) {
            if (filter == 'A') {
            	int sum[3] = {0, 0, 0};
            	int tempX, tempY;

            	for (int i = 0; i < size; i++) {
            		for (int j = 0; j < size; j++) {
            			tempX = x + j - (size / 2);
            			tempY = y + i - (size / 2);

            			if (tempX >= 0 && tempX < img->GetWidth()
            					&& tempY >= 0 && tempY < img->GetHeight()) {
							sum[0] += img->GetR(tempX, tempY);
							sum[1] += img->GetG(tempX, tempY);
							sum[2] += img->GetB(tempX, tempY);
            			}
            		}
            	}

            	imgMod->SetRGB( x, y, sum[0] / 9, sum[1] / 9, sum[2] / 9);
            }
            if(filter == 'G') imgMod->SetRGB( x, y, img->GetR(x,y),0,0);
            if(filter == 'S') imgMod->SetRGB( x, y, 0,img->GetG(x,y),0);
            if(filter == 'P') imgMod->SetRGB( x, y, 0,0,img->GetB(x,y));
         }
      }

      //imgMod->SetDataAsMatrix(m);
      //imgMod->DeallocatePixelMatrix(&m, imgMod->GetHeight(), imgMod->GetWidth());
   }
}

void control(int value)
{
   if(value == 1)
   {
      img->Read(input);
      imgMod->Copy(img);
      glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());
   }
   if(value == 2)
   {
      imgMod->Save(output);
   }

   if(value == 3) { // Radio button
	   if (option == 0) imgMod->Copy(img);
	   if (option == 1) modifyImage('A', brightness);
	   if (option == 2) modifyImage('G', brightness);
	   if (option == 3) modifyImage('S', brightness);
	   if (option == 4) modifyImage('P', brightness);
   }
   if(value == 4)
   {
      if( brightness > prevBrightness)
         imgMod->AddValueToChannels(1);
      else
         imgMod->AddValueToChannels(-1);
      prevBrightness = brightness;
   }
}

static void key(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27 :
        case 'q':
            exit(0);
        break;
        case 'm':
            imgMod->Copy(img);
            modifyImage('R', 1);
        break;
    }
    glutPostRedisplay();
}

int main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   strcpy(input, "figs/woman.png");
   strcpy(output, "figs/output.png");

   img    = new PixelLab(input);
   imgMod = new PixelLab();
   imgMod->Copy(img);

   glutInitWindowSize(img->GetWidth(),img->GetHeight());
   glutInitWindowPosition(100,100);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

   main_window = glutCreateWindow("Image Filtering in the Spatial Domain");

   glutKeyboardFunc(key);
   glutIdleFunc( idle);
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);

   // GLUI
   // Use the first line above to get the interface in the same window as the graphics
   // Use the second line (comment the first) to get the interface in a separeted window
   GLUI *glui = GLUI_Master.create_glui_subwindow( main_window, GLUI_SUBWINDOW_RIGHT );
   //GLUI *glui = GLUI_Master.create_glui( "" );

   GLUI_Master.set_glutReshapeFunc(reshape);

   GLUI_Panel *io_panel = glui->add_panel( (char *) "Input/Output" );
   GLUI_EditText *edit1, *edit2;
   edit1 = glui->add_edittext_to_panel (io_panel, (char *) "", GLUI_EDITTEXT_TEXT, input);
   edit2 = glui->add_edittext_to_panel (io_panel, (char *) "", GLUI_EDITTEXT_TEXT, output);
   edit1->set_w(185);
   edit2->set_w(185);
   glui->add_column_to_panel( io_panel, false);
   GLUI_Button *b1 = glui->add_button_to_panel( io_panel, (char*) "Load", 1, control );
   GLUI_Button *b2 = glui->add_button_to_panel( io_panel, (char*) "Save", 2, control );
   b1->set_w(50);
   b2->set_w(50);

   GLUI_Panel *channel_panel = glui->add_panel((char *) "Filters" );
   //glui->add_statictext_to_panel( channel_panel, (char *) "Low-pass");
   radio = glui->add_radiogroup_to_panel(channel_panel, &option, 3, control);
   glui->add_radiobutton_to_group( radio, (char *) "No filter" );
   glui->add_radiobutton_to_group( radio, (char *) "Average" );
   glui->add_radiobutton_to_group( radio, (char *) "Gaussian" );
   glui->add_radiobutton_to_group( radio, (char *) "Sobel" );
   glui->add_radiobutton_to_group( radio, (char *) "Prewitt" );
   glui->add_separator_to_panel( channel_panel );
   GLUI_Spinner* spinner;
   spinner = glui->add_spinner_to_panel( channel_panel, (char*)"Increase/Decrease brightness", GLUI_SPINNER_INT, &brightness, 4, control);

   glui->add_button((char *) "Quit", 0,(GLUI_Update_CB)exit );

   glui->set_main_gfx_window( main_window );
   GLUI_Master.set_glutIdleFunc( idle );
   computeUIWidth(); // Compute the size of the user interface
   glutReshapeWindow(imgMod->GetWidth() + UI_width, imgMod->GetHeight());

   glutMainLoop();

   return 1;
}
