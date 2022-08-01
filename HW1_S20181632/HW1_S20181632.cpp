#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
int window_width = 500, window_height = 500; // initial window size
int prev_x = 0, prev_y = 0;
int rightbuttonpressed = 0; //if right mouse is pressed
int shiftpressed = 0; //flag whether shift is presssed
float r = 255.0f / 255.0f, g = 204.0f / 255.0f, b = 255.0f / 255.0f; // Background color = Light Purple
float arr[128][2];//array to save converted coordinates
float ctr_x, ctr_y;//coordinates of polygon centroid
int count = 0;//number of points
int mk_polygon = 0;//flag whether making polygon is in progress
int fin_poly = 0;//flag whether polygon is made
int cnt = 0;//flag if centroid is found
float area = 0;//component used to find centroid
float temp = 0;//component used to find centroid
int animation_mode = 0;
int rot = 0;//whether polygon is rotating

void display(void) {
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glPointSize(7.0f);
	glBegin(GL_POINTS);

	for (int i = 0;i < count;i++) { //Draw points
		glColor3f(255.0f / 255.0f, 102.0f / 255.0f, 178.0f / 255.0f);
		glVertex2f(arr[i][0], arr[i][1]);
	}
	if (cnt&&rot) {//Draw centroid
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(ctr_x, ctr_y);
	}
	glEnd();


	glLineWidth(2.0f);
	glBegin(GL_LINES);
	float pre_x = arr[0][0];
	float pre_y = arr[0][1];
	for (int i = 1;i < count; i++) { //Draw lines
		glColor3f(102.0f / 255.0f, 102.0f / 255.0f, 255.0f / 255.0f);
		glVertex2f(pre_x, pre_y); glVertex2f(arr[i][0], arr[i][1]);
		pre_x = arr[i][0];
		pre_y = arr[i][1];
	}
	if (mk_polygon) { // Make polygon
		glVertex2f(arr[0][0], arr[0][1]); glVertex2f(pre_x, pre_y);
		fin_poly = 1;
	}
	glEnd();
	glFlush();
}

void timer(int value) {
	if (animation_mode) {	
		//glTranslatef(-ctr_x, -ctr_y, 0.0);
		//glRotatef(45.0f,0,0,1);
		//glTranslatef(ctr_x, ctr_y, 0.0);
		
		for (int i = 0; i < count;i++) {
			float tp = arr[i][0] = arr[i][0] - ctr_x;
			arr[i][1] = arr[i][1] - ctr_y;
			arr[i][0] = arr[i][0] * cos(5.0f*(3.141592/180.0f)) - arr[i][1] * sin(5.0f * (3.141592 / 180.0f));
			arr[i][1] = tp * sin(5.0f * (3.141592 / 180.0f)) + arr[i][1] * cos(5.0f * (3.141592 / 180.0f));
			arr[i][0] = arr[i][0] + ctr_x;
			arr[i][1] = arr[i][1] + ctr_y;
		}
		glutPostRedisplay();
		if (!rot) { animation_mode = 0; }
		glutTimerFunc(100, timer, 0);
	}
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'p':
		if (count < 3) { //when there are less then 3 points, print error
			fprintf(stdout, "Lack of points.\n");
		}
		else {
			mk_polygon = 1; //when it is sufficient to make polygon
			glutPostRedisplay();
		}
		break;
	case 'c':
		if (rot) { // When its rotating, 'c'command should freeze
			break;
		}
		for (int i = 0;i < count;i++) {// reset the coordinates
			for (int j = 0;j <= 1;j++) {
				arr[i][j] = NULL;
			}
		}
		// reset all the flags and values
		ctr_x = NULL;
		ctr_y = NULL;
		cnt = 0;
		mk_polygon = 0;
		fin_poly = 0;
		count = 0;
		area = 0;
		glutPostRedisplay();

		break;
	case 'r':
		if (!cnt && fin_poly) {//when there are no centroid marked and polygon is drawn, draw centroid
			ctr_x = 0;
			ctr_y = 0;
			arr[count][0] = arr[0][0];
			arr[count][1] = arr[0][1];
			for (int i = 0; i < count; i++) {
				area += arr[i][0] * arr[i + 1][1] - arr[i + 1][0] * arr[i][1];
			}
			area *= 0.5f;
			for (int i = 0; i < count; i++) {
				temp = arr[i][0] * arr[i + 1][1] - arr[i + 1][0] * arr[i][1];
				ctr_x += (arr[i][0] + arr[i + 1][0]) * temp;
				ctr_y += (arr[i][1] + arr[i + 1][1]) * temp;
			}
			ctr_x = ctr_x / (6.0f * area);
			ctr_y = ctr_y / (6.0f * area);
			cnt = 1;
			animation_mode = 1;
			rot = 1;
			glutTimerFunc(100, timer, 0);
			glutPostRedisplay();
		}
		else if (!rot && cnt) {
			rot = 1;
			animation_mode = 1;
			glutTimerFunc(100, timer, 0);
			glutPostRedisplay();
		}
		else if (rot) {
			area = 0;
			rot = 0;
			glutPostRedisplay();
		}
		else if (count < 3 || !fin_poly) { //if polygon is not done or l
			fprintf(stdout, "No Polygon exists\n");
		}
		break;
	case 'f': //exit
		glutLeaveMainLoop();
		break;
	}
}


void special(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT:
		if (fin_poly&&!rot) { //move left, negative x value
			for (int i = 0; i < count;i++) {
				arr[i][0] -= 0.05f;
				
			}
			if (cnt) {
					ctr_x -= 0.05f;
				}
		}
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		if (fin_poly && !rot) { //move right, positive x value
			for (int i = 0; i < count;i++) {
				arr[i][0] += 0.05f;
			}
			if (cnt) {
				ctr_x += 0.05f;
			}
		}
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		if (fin_poly && !rot) { //move down, negative y value
			for (int i = 0; i < count;i++) {
				arr[i][1] -= 0.05f;
			}
			if (cnt) {
				ctr_y -= 0.05f;
			}
		}
		glutPostRedisplay();
		break;
	case GLUT_KEY_UP:
		if (fin_poly && !rot) { //move up, positive y value
			for (int i = 0; i < count;i++) {
				arr[i][1] += 0.05f;
			}
			if (cnt) {
				ctr_y += 0.05f;
			}
		}
		glutPostRedisplay();
		break;
	}
}

void mousepress(int button, int state, int x, int y) {
	shiftpressed = glutGetModifiers(); //whether shift if pressed
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN) && (shiftpressed == GLUT_ACTIVE_SHIFT) && (!fin_poly)) {
		//active when polygon is incomplete and shift button is press
		float x1 = x;
		float y1 = y;
		if (x < window_width / 2)
		{
			arr[count][0] = 0.0f - (1 - (x1 / (window_width / 2)));
		}
		else if (x > window_width / 2)
		{
			arr[count][0] = (x1 / (window_width / 2)) - 1.0f;
		}
		else
			arr[count][0] = 0;
		if (y < window_height / 2)
		{
			arr[count][1] = (1 - (y1 / (window_height / 2)));
		}
		else if (y > window_height / 2)
		{
			arr[count][1] = 0.0f - ((y1 / (window_height / 2)) - 1.0f);
		}
		else
			arr[count][1] = 0;
		count++;
		shiftpressed = 0;
	}
	else if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN)) {
		if (!rot) {
			rightbuttonpressed = 1;
			prev_x = x, prev_y = y;
		}
		
	}
	else if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_UP))
		rightbuttonpressed = 0;
}

void mousemove(int x, int y) {
	if (rightbuttonpressed && fin_poly && !rot) {// when right button is pressed and
		float x1 = x;					 // polygon is complete, move polygon 
		float y1 = y;					 // distance the mouse moved
		float mov_x = prev_x - x1;
		float mov_y = prev_y - y1;
		prev_x = x;
		prev_y = y;
		mov_x = mov_x / (window_width / 2);
		mov_y = mov_y / (window_height / 2);
		for (int i = 0;i < count;i++) {
			arr[i][0] -= mov_x;
			arr[i][1] += mov_y;
		}
		ctr_x -= mov_x;
		ctr_y += mov_y;
		glutPostRedisplay();
	}
}

void reshape(int width, int height) {
	window_width = width, window_height = height;
	glViewport(0, 0, width, height);
}

void close(void) {
	fprintf(stdout, "\n^^^ The control is at the close callback function now.\n\n");
}


void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMouseFunc(mousepress);
	glutMotionFunc(mousemove);
	glutReshapeFunc(reshape);
	glutTimerFunc(10, timer, 0);
	glutCloseFunc(close);
}

void initialize_renderer(void) {
	register_callbacks();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = TRUE;
	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void greetings(char* program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 4
void main(int argc, char* argv[]) {
	char program_name[64] = "CSE4170 HomeWork 1 coded by 20181632";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'p', 'c', 'f', 'r'",
		"    - Special keys used: LEFT, RIGHT, UP, DOWN",
		"    - Mouse used: L-click, R-click and move",
		"    - Other operations: window size change"
	};

	glutInit(&argc, argv);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE); 

	glutInitDisplayMode(GLUT_RGBA);

	glutInitWindowSize(500, 500);
	glutInitWindowPosition(500, 200);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	glutMainLoop();
	fprintf(stdout, "^^^ The control is at the end of main function now.\n\n");
}