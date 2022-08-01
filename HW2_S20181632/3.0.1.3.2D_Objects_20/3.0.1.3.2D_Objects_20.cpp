#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <math.h>

#include "Shaders/LoadShaders.h"
GLuint h_ShaderProgram; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

														  // include glm/*.hpp only if necessary
														  //#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, ortho, etc.
glm::mat4 ModelViewProjectionMatrix;
glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0

int win_width = 0, win_height = 0;
float centerx = 0.0f, centery = 0.0f, rotate_angle = 0.0f;
int animation_mode = 1;
unsigned int timestamp = 0;
unsigned int apple_r = 1;

GLfloat axes[3][2];
GLfloat axes_color[2][3] = { 
	{ 102.0 / 255.0f, 51.0 / 255.0f, 0.0f },
	{ 96.0f / 255.0f,  96.0f / 255.0f,  96.0f / 255.0f} 
};
GLuint VBO_axes, VAO_axes;

void prepare_axes(void) { // Draw axes in their MC.
	axes[0][0] = -win_width / 2.0f; axes[0][1] = -300.0f;
	axes[1][0] = win_width / 2.0f; axes[1][1] = -300.0f;
	axes[2][0] = 200.0f; axes[2][1] = -295.0f;
	//axes[2][0] = 0.0f; axes[2][1] = -win_height / 2.5f;
	//axes[3][0] = 0.0f; axes[3][1] = win_height / 2.5f;

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_axes);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_axes);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_axes);
	glBindVertexArray(VAO_axes);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_axes);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void update_axes(void) {
	axes[0][0] = -win_width / 2.0f; axes[1][0] = win_width / 2.0f;
	axes[0][1] = -win_height / 2.0f + 100.0f; axes[1][1] = -win_height / 2.0f + 100.0f;
	//axes[2][1] = -win_height / 2.25f;
	//axes[3][1] = win_height / 2.25f;

	glBindBuffer(GL_ARRAY_BUFFER, VBO_axes);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_axes(void) {
	glUniform3fv(loc_primitive_color, 1, axes_color[0]);
	glBindVertexArray(VAO_axes);
	glLineWidth(5.0);
	glDrawArrays(GL_LINES, 0, 2);

	glUniform3fv(loc_primitive_color, 1, axes_color[1]);
	glBindVertexArray(VAO_axes);
	glPointSize(13.0);
	glDrawArrays(GL_POINTS, 2, 1);
	glBindVertexArray(0);
}

GLfloat line[2][2];
GLfloat line_color[3] = { 1.0f, 0.0f, 0.0f };
GLuint VBO_line, VAO_line;

void prepare_line(void) { 	// y = x - win_height/4
	line[0][0] = (1.0f / 4.0f - 1.0f / 2.5f)*win_height;
	line[0][1] = (1.0f / 4.0f - 1.0f / 2.5f)*win_height - win_height / 4.0f;
	line[1][0] = win_width / 2.5f;
	line[1][1] = win_width / 2.5f - win_height / 4.0f;

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_line);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_line);
	glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_line);
	glBindVertexArray(VAO_line);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_line);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void update_line(void) { 	// y = x - win_height/4
	line[0][0] = (1.0f / 4.0f - 1.0f / 2.5f)*win_height;
	line[0][1] = (1.0f / 4.0f - 1.0f / 2.5f)*win_height - win_height / 4.0f;
	line[1][0] = win_width / 2.5f;
	line[1][1] = win_width / 2.5f - win_height / 4.0f;

	glBindBuffer(GL_ARRAY_BUFFER, VBO_line);
	glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_line(void) { // Draw line in its MC.
					   // y = x - win_height/4
	glUniform3fv(loc_primitive_color, 1, line_color);
	glBindVertexArray(VAO_line);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
}

#define AIRPLANE_BIG_WING 0
#define AIRPLANE_SMALL_WING 1
#define AIRPLANE_BODY 2
#define AIRPLANE_BACK 3
#define AIRPLANE_SIDEWINDER1 4
#define AIRPLANE_SIDEWINDER2 5
#define AIRPLANE_CENTER 6
GLfloat big_wing[6][2] = { { 0.0, 0.0 },{ -20.0, 15.0 },{ -20.0, 20.0 },{ 0.0, 23.0 },{ 20.0, 20.0 },{ 20.0, 15.0 } };
GLfloat small_wing[6][2] = { { 0.0, -18.0 },{ -11.0, -12.0 },{ -12.0, -7.0 },{ 0.0, -10.0 },{ 12.0, -7.0 },{ 11.0, -12.0 } };
GLfloat body[5][2] = { { 0.0, -25.0 },{ -6.0, 0.0 },{ -6.0, 22.0 },{ 6.0, 22.0 },{ 6.0, 0.0 } };
GLfloat back[5][2] = { { 0.0, 25.0 },{ -7.0, 24.0 },{ -7.0, 21.0 },{ 7.0, 21.0 },{ 7.0, 24.0 } };
GLfloat sidewinder1[5][2] = { { -20.0, 10.0 },{ -18.0, 3.0 },{ -16.0, 10.0 },{ -18.0, 20.0 },{ -20.0, 20.0 } };
GLfloat sidewinder2[5][2] = { { 20.0, 10.0 },{ 18.0, 3.0 },{ 16.0, 10.0 },{ 18.0, 20.0 },{ 20.0, 20.0 } };
GLfloat center[1][2] = { { 0.0, 0.0 } };
GLfloat airplane_color[7][3] = {
	{ 150 / 255.0f, 129 / 255.0f, 183 / 255.0f },  // big_wing
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // small_wing
	{ 111 / 255.0f,  85 / 255.0f, 157 / 255.0f },  // body
	{ 150 / 255.0f, 129 / 255.0f, 183 / 255.0f },  // back
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // sidewinder1
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // sidewinder2
	{ 255 / 255.0f,   0 / 255.0f,   0 / 255.0f }   // center
};

GLuint VBO_airplane, VAO_airplane;

int airplane_clock = 0;
float airplane_s_factor = 1.0f;

void prepare_airplane() {
	GLsizeiptr buffer_size = sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1) + sizeof(sidewinder2) + sizeof(center);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_airplane);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(big_wing), big_wing);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing), sizeof(small_wing), small_wing);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing), sizeof(body), body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body), sizeof(back), back);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back),
		sizeof(sidewinder1), sidewinder1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1), sizeof(sidewinder2), sidewinder2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1) + sizeof(sidewinder2), sizeof(center), center);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_airplane);
	glBindVertexArray(VAO_airplane);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_airplane() { // Draw airplane in its MC.
	glBindVertexArray(VAO_airplane);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BIG_WING]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SMALL_WING]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BACK]);
	glDrawArrays(GL_TRIANGLE_FAN, 17, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER1]);
	glDrawArrays(GL_TRIANGLE_FAN, 22, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER2]);
	glDrawArrays(GL_TRIANGLE_FAN, 27, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_CENTER]);
	glPointSize(5.0);
	glDrawArrays(GL_POINTS, 32, 1);
	glPointSize(1.0);
	glBindVertexArray(0);
}



	

//tree
#define TRUNK 0
#define LEAF 1
#define PATTERN 2

GLfloat tree_trunk[4][2] = { { -10.0, -50.0 },{ -10.0, 10.0 },{ 10.0, 10.0 },{ 10.0, -50.0 } };
GLfloat tree_leaves[8][2] = { { -20.0, 5.0 },{ -30.0, 19.0 },{ -20.0, 33.0 },{ 0.0, 40.0 }, { 20.0, 33.0 },{ 30.0, 19.0 },{ 20.0, 5.0 },{ 0.0, -1 } };
GLfloat outer_p[12][2] = { {-10.0,-10.0},{-6.0,-12.0},{-6.0,-12.0},{-2.0,-14.0},{-2.0,-14.0},{2.0,-16.0},{2.0,-16.0},{-2.0,-18.0},{-2.0,-18.0},{-6.0 ,-20.0},{-6.0,-20.0},{-10.0,-22.0} };
GLfloat middle_p[8][2] = { {-10.0,-13.0},{-7.0,-15.0},{-7.0,-15.0},{-4.0,-16.0},{-4.0,-16.0},{-7.0,-17.0},{-7.0,-17.0},{-10.0,-19.0} };
GLfloat inner_p[4][2] = { {-10.0,-15.0},{-8.0,-16.0},{-8.0,-16.0},{-10.0,-17.0} };

GLfloat tree_color[3][3] = {
	{ 153.0f / 255.0f, 76.0f / 255.0f, 0.0f / 255.0f},
	{ 0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f},
	{ 102.0f / 255.0f, 51.0f / 255.0f, 0.0f / 255.0f}
};

GLuint VBO_tree, VAO_tree;
void prepare_tree() {
	GLsizeiptr buffer_size = sizeof(tree_trunk) + sizeof(tree_leaves) + sizeof(outer_p) + sizeof(middle_p) + sizeof(inner_p);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_tree);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_tree);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tree_trunk), tree_trunk);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(tree_trunk), sizeof(tree_leaves), tree_leaves);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(tree_trunk) + sizeof(tree_leaves), sizeof(outer_p), outer_p);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(tree_trunk) + sizeof(tree_leaves) + sizeof(outer_p), sizeof(middle_p),middle_p);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(tree_trunk) + sizeof(tree_leaves) + sizeof(outer_p) + sizeof(middle_p), sizeof(inner_p),inner_p);
	
	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_tree);
	glBindVertexArray(VAO_tree);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_tree);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));


	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_tree() {
	glBindVertexArray(VAO_tree);

	glUniform3fv(loc_primitive_color, 1, tree_color[TRUNK]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, tree_color[LEAF]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 8);

	glUniform3fv(loc_primitive_color, 1, tree_color[PATTERN]);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINES, 12, 12);

	glUniform3fv(loc_primitive_color, 1, tree_color[PATTERN]);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINES, 24, 8);

	glUniform3fv(loc_primitive_color, 1, tree_color[PATTERN]);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINES, 32, 14);

	glBindVertexArray(0);
}

//cloud

#define C_COLOR 0

GLfloat c_body[4][2] = { {-26.0,13.0},{26.0,13.0},{26.0,-13.0},{-26.0,-13.0} };
GLfloat left_up[8][2] = { {-26.0,13.0},{-26.0,23.0},{-20.0,26.0},{-16.0,30.0},{-10.0,30.0},{-6.0,26.0},{0.0,23.0},{0.0,13.0} };
GLfloat right_up[8][2] = { {0.0,13.0},{0.0,23.0},{6.0,26.0},{10.0,30.0},{16.0,30.0},{20.0,26.0},{26.0,23.0},{26.0,13.0} };
GLfloat right_p[8][2] = { {26.0,13.0},{36.0,13.0},{39.0,7.0},{43.0,3.0},{43.0,-3.0},{39.0,-7.0},{36.0,-13.0},{26.0,-13.0} };
GLfloat left_p[8][2] = { {-26.0,13.0},{-36.0,13.0},{-39.0,7.0},{-43.0,3.0},{-43.0,-3.0},{-39.0,-7.0},{-36.0,-13.0},{-26.0,-13.0} };
GLfloat left_down[8][2] = { {-26.0,-13.0},{-26.0,-23.0},{-20.0,-26.0},{-16.0,-30.0},{-10.0,-30.0},{-6.0,-26.0},{0.0,-23.0},{0.0,-13.0} };
GLfloat right_down[8][2] = { {0.0,-13.0},{0.0,-23.0},{6.0,-26.0},{10.0,-30.0},{16.0,-30.0},{20.0,-26.0},{26.0,-23.0},{26.0,-13.0} };
GLfloat cloud_color[1][3] = {
	{ 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f },  // big_wing
	
};

GLuint VBO_cloud, VAO_cloud;



void prepare_cloud() {
	GLsizeiptr buffer_size = sizeof(c_body) + sizeof(left_up) + sizeof(right_up) +sizeof(left_p) + sizeof(right_p) + sizeof(left_down) + sizeof(right_down);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_cloud);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_cloud);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c_body), c_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(c_body), sizeof(left_up), left_up);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(c_body) + sizeof(left_up), sizeof(right_up), right_up);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(c_body) + sizeof(left_up) + sizeof(right_up), sizeof(left_p), left_p);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(c_body) + sizeof(left_up) + sizeof(right_up) + sizeof(left_p),
		sizeof(right_p), right_p);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(c_body) + sizeof(left_up) + sizeof(right_up) + sizeof(left_p)
		+ sizeof(right_p), sizeof(left_down), left_down);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(c_body) + sizeof(left_up) + sizeof(right_up) + sizeof(left_p)
		+ sizeof(right_p)+ sizeof(left_down), sizeof(right_down),right_down);
		

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_cloud);
	glBindVertexArray(VAO_cloud);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_cloud);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_cloud() { // Draw airplane in its MC.
	glBindVertexArray(VAO_cloud);

	glUniform3fv(loc_primitive_color, 1,cloud_color[C_COLOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, cloud_color[C_COLOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 8);

	glUniform3fv(loc_primitive_color, 1, cloud_color[C_COLOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 8);

	glUniform3fv(loc_primitive_color, 1, cloud_color[C_COLOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 8);

	glUniform3fv(loc_primitive_color, 1, cloud_color[C_COLOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 28, 8);

	glUniform3fv(loc_primitive_color, 1, cloud_color[C_COLOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 8);

	glUniform3fv(loc_primitive_color, 1, cloud_color[C_COLOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 44, 8);
	
	glBindVertexArray(0);
}
//apple
#define APPLE 0
#define HOLE 1
#define STEM 2

GLfloat apple[12][2] = { { -3.0, -9.0 },{-6.0,-7.0}, { -9.0, -3.0 },{ -9.0, 3.0 },{-6.0 ,7.0},{ -3.0, 9.0 },{ 3.0, 9.0 },{6.0,7.0}, { 9.0, 3.0 },{ 9.0,-3.0 },{6.0,-7.0}, { 3.0, -9.0 } };
GLfloat hole[6][2] = { { -4.0, 8.0 },{ -3.0, 6.0 },{ -3.0, 6.0 },{ 3.0, 6.0 }, { 3.0, 6.0 },{ 4.0, 8.0 } };
GLfloat stem[2][2] = { {0.0,8.0},{2.0,11.0} };

GLfloat apple_color[3][3] = {
	{ 255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f},
	{ 102.0f / 255.0f, 51.0f / 255.0f, 0.0f / 255.0f},
	{ 102.0f / 255.0f, 51.0f / 255.0f, 0.0f / 255.0f}
};

GLuint VBO_apple, VAO_apple;
void prepare_apple() {
	GLsizeiptr buffer_size = sizeof(apple) + sizeof(hole) + sizeof(stem);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_apple);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_apple);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(apple), apple);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(apple), sizeof(hole), hole);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(apple) + sizeof(hole), sizeof(stem), stem);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_apple);
	glBindVertexArray(VAO_apple);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_apple);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_apple() {
	glBindVertexArray(VAO_apple);

	glUniform3fv(loc_primitive_color, 1, apple_color[APPLE]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 12);

	glUniform3fv(loc_primitive_color, 1, apple_color[HOLE]);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINES, 12, 6);

	glUniform3fv(loc_primitive_color, 1, apple_color[STEM]);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINES, 18, 2);

	glBindVertexArray(0);
}

//person
#define PERSON_HEAD 0
#define PERSON_NECK 1
#define PERSON_BODY 2
#define PERSON_LEFT_SHOULDER 3
#define PERSON_RIGHT_SHOULDER 4
#define PERSON_LEFT_ARM 5
#define PERSON_RIGHT_ARM 6
#define PERSON_LEFT_LEG 7
#define PERSON_RIGHT_LEG 8
#define EYES 9

GLfloat head[4][2] = { {-15.0,95.0},{15.0,95.0},{15.0,75.0},{-15.0,75.0} };
GLfloat p_neck[4][2] = { {-6.0,75.0},{6.0,75.0},{6.0,70.0},{-6.0,70.0} };
GLfloat p_body[4][2] = { {-20.0,70.0},{20.0,70.0},{20.0,40.0},{-20.0,40.0} };
GLfloat left_sh[4][2] = { {-30.0,70.0},{-20.0,70.0},{-20.0,60.0 }, { -30.0,60.0 }};
GLfloat right_sh[4][2] = { {20.0,70.0},{30.0,70.0},{30.0,60.0},{20.0,60.0} };
GLfloat left_ar[4][2] = { {-30.0,60.0},{-25.0,60.0},{-25.0,30.0},{-30.0,30.0} };
GLfloat right_ar[4][2] = { {25.0,60.0},{30.0,60.0},{30.0,30.0},{25.0,30.0 } };
GLfloat left_lg[4][2] = { {-20.0,40.0},{-2.0,40.0},{-2.0,0.0},{-20.0,0.0} };
GLfloat right_lg[4][2] = { {2.0,40.0},{20.0,40.0},{20.0,0.0},{2.0,0.0} };
GLfloat eyes[2][2] = { {-7.0,85.0},{7.0,85.0} };

GLfloat person_color[10][3] = {
	{ 255.0f / 255.0f, 204.0f / 255.0f, 153.0f / 255.0f },  // head
	{ 255.0f / 255.0f, 204.0f / 255.0f, 153.0f / 255.0f },  // neck
	{ 255.0f / 255.0f,  255.0f / 255.0f, 255.0f / 255.0f },  // body
	{ 255.0f / 255.0f,  255.0f / 255.0f, 255.0f / 255.0f },  // left shoulder
	{ 255.0f / 255.0f,  255.0f / 255.0f, 255.0f / 255.0f }, // right shoulder
	{ 255.0f / 255.0f,  255.0f / 255.0f, 255.0f / 255.0f }, // left arm
	{ 255.0f / 255.0f,  255.0f / 255.0f, 255.0f / 255.0f }, // right arm
	{ 0.0f / 255.0f,  0.0f / 0.0f, 0.0f / 255.0f }, // left leg
	{ 0.0f / 255.0f,  0.0f / 0.0f, 0.0f / 255.0f }, // right leg
	{ 0.0f / 255.0f,  0.0f / 0.0f, 0.0f / 255.0f }, // eyes
};

GLuint VBO_person, VAO_person;



void prepare_person() {
	GLsizeiptr buffer_size = sizeof(head) + sizeof(p_neck) + sizeof(p_body) + sizeof(left_sh)
		+ sizeof(right_sh) + sizeof(left_ar) + sizeof(right_ar) + sizeof(left_lg) + sizeof(right_lg) + sizeof(eyes);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_person);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_person);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(head), head);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head), sizeof(p_neck), p_neck);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck), sizeof(p_body), p_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck) + sizeof(p_body), sizeof(left_sh), left_sh);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck) + sizeof(p_body) + sizeof(left_sh), sizeof(right_sh), right_sh);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck) + sizeof(p_body) + sizeof(left_sh) + sizeof(right_sh),
		sizeof(left_ar), left_ar);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck) + sizeof(p_body) + sizeof(left_sh) + sizeof(right_sh) +
		sizeof(left_ar), sizeof(right_ar), right_ar);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck) + sizeof(p_body) + sizeof(left_sh) + sizeof(right_sh) +
		sizeof(left_ar) + sizeof(right_ar), sizeof(left_lg), left_lg);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck) + sizeof(p_body) + sizeof(left_sh) + sizeof(right_sh) +
		sizeof(left_ar) + sizeof(right_ar) + sizeof(left_lg) , sizeof(right_lg), right_lg);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(head) + sizeof(p_neck) + sizeof(p_body) + sizeof(left_sh) + sizeof(right_sh) +
		sizeof(left_ar) + sizeof(right_ar) + sizeof(left_lg) + sizeof(right_lg) ,sizeof(eyes), eyes);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_person);
	glBindVertexArray(VAO_person);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_person);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_person() { // Draw airplane in its MC.
	glBindVertexArray(VAO_person);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_HEAD]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_NECK]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_LEFT_SHOULDER]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_RIGHT_SHOULDER]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_LEFT_ARM]);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_RIGHT_ARM]);
	glDrawArrays(GL_TRIANGLE_FAN, 24, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_LEFT_LEG]);
	glDrawArrays(GL_TRIANGLE_FAN, 28, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[PERSON_RIGHT_LEG]);
	glDrawArrays(GL_TRIANGLE_FAN, 32, 4);

	glUniform3fv(loc_primitive_color, 1, person_color[EYES]);
	glPointSize(5.0);
	glDrawArrays(GL_POINTS, 36 , 2);
	//glPointSize(1.0);

	
	glBindVertexArray(0);
}
//car
#define CAR_BODY 0
#define CAR_FRAME 1
#define CAR_WINDOW 2
#define CAR_LEFT_LIGHT 3
#define CAR_RIGHT_LIGHT 4
#define CAR_LEFT_WHEEL 5
#define CAR_RIGHT_WHEEL 6

GLfloat car_body[4][2] = { { -16.0, -8.0 },{ -16.0, 0.0 },{ 16.0, 0.0 },{ 16.0, -8.0 } };
GLfloat car_frame[4][2] = { { -10.0, 0.0 },{ -10.0, 10.0 },{ 10.0, 10.0 },{ 10.0, 0.0 } };
GLfloat car_window[4][2] = { { -8.0, 0.0 },{ -8.0, 8.0 },{ 8.0, 8.0 },{ 8.0, 0.0 } };
GLfloat car_left_light[4][2] = { { -9.0, -6.0 },{ -10.0, -5.0 },{ -9.0, -4.0 },{ -8.0, -5.0 } };
GLfloat car_right_light[4][2] = { { 9.0, -6.0 },{ 8.0, -5.0 },{ 9.0, -4.0 },{ 10.0, -5.0 } };
GLfloat car_left_wheel[4][2] = { { -10.0, -12.0 },{ -10.0, -8.0 },{ -6.0, -8.0 },{ -6.0, -12.0 } };
GLfloat car_right_wheel[4][2] = { { 6.0, -12.0 },{ 6.0, -8.0 },{ 10.0, -8.0 },{ 10.0, -12.0 } };

GLfloat car_color[7][3] = {
	{ 0 / 255.0f, 149 / 255.0f, 159 / 255.0f },
	{ 0 / 255.0f, 149 / 255.0f, 159 / 255.0f },
	{ 216 / 255.0f, 208 / 255.0f, 174 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 21 / 255.0f, 30 / 255.0f, 26 / 255.0f },
	{ 21 / 255.0f, 30 / 255.0f, 26 / 255.0f }
};

GLuint VBO_car, VAO_car;
void prepare_car() {
	GLsizeiptr buffer_size = sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light) + sizeof(car_left_wheel) + sizeof(car_right_wheel);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_car);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(car_body), car_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body), sizeof(car_frame), car_frame);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame), sizeof(car_window), car_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window), sizeof(car_left_light), car_left_light);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light),
		sizeof(car_right_light), car_right_light);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light), sizeof(car_left_wheel), car_left_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light) + sizeof(car_left_wheel), sizeof(car_right_wheel), car_right_wheel);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_car);
	glBindVertexArray(VAO_car);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_car() {
	glBindVertexArray(VAO_car);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_FRAME]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_LEFT_LIGHT]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_RIGHT_LIGHT]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_LEFT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_RIGHT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 24, 4);

	glBindVertexArray(0);
}

//draw cocktail
#define COCKTAIL_NECK 0
#define COCKTAIL_LIQUID 1
#define COCKTAIL_REMAIN 2
#define COCKTAIL_STRAW 3
#define COCKTAIL_DECO 4

GLfloat neck[6][2] = { { -6.0, -12.0 },{ -6.0, -11.0 },{ -1.0, 0.0 },{ 1.0, 0.0 },{ 6.0, -11.0 },{ 6.0, -12.0 } };
GLfloat liquid[6][2] = { { -1.0, 0.0 },{ -9.0, 4.0 },{ -12.0, 7.0 },{ 12.0, 7.0 },{ 9.0, 4.0 },{ 1.0, 0.0 } };
GLfloat remain[4][2] = { { -12.0, 7.0 },{ -12.0, 10.0 },{ 12.0, 10.0 },{ 12.0, 7.0 } };
GLfloat straw[4][2] = { { 7.0, 7.0 },{ 12.0, 12.0 },{ 14.0, 12.0 },{ 9.0, 7.0 } };
GLfloat deco[8][2] = { { 12.0, 12.0 },{ 10.0, 14.0 },{ 10.0, 16.0 },{ 12.0, 18.0 },{ 14.0, 18.0 },{ 16.0, 16.0 },{ 16.0, 14.0 },{ 14.0, 12.0 } };

GLfloat cocktail_color[5][3] = {
	{ 235 / 255.0f, 225 / 255.0f, 196 / 255.0f },
	{ 0 / 255.0f, 63 / 255.0f, 122 / 255.0f },
	{ 235 / 255.0f, 225 / 255.0f, 196 / 255.0f },
	{ 191 / 255.0f, 255 / 255.0f, 0 / 255.0f },
	{ 218 / 255.0f, 165 / 255.0f, 32 / 255.0f }
};

GLuint VBO_cocktail, VAO_cocktail;
void prepare_cocktail() {
	GLsizeiptr buffer_size = sizeof(neck) + sizeof(liquid) + sizeof(remain) + sizeof(straw)
		+ sizeof(deco);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_cocktail);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_cocktail);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(neck), neck);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck), sizeof(liquid), liquid);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid), sizeof(remain), remain);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid) + sizeof(remain), sizeof(straw), straw);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid) + sizeof(remain) + sizeof(straw),
		sizeof(deco), deco);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_cocktail);
	glBindVertexArray(VAO_cocktail);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_cocktail);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_cocktail() {
	glBindVertexArray(VAO_cocktail);

	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_NECK]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_LIQUID]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_REMAIN]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_STRAW]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_DECO]);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 8);

	glBindVertexArray(0);
}

//draw car2
#define CAR2_BODY 0
#define CAR2_FRONT_WINDOW 1
#define CAR2_BACK_WINDOW 2
#define CAR2_FRONT_WHEEL 3
#define CAR2_BACK_WHEEL 4
#define CAR2_LIGHT1 5
#define CAR2_LIGHT2 6

GLfloat car2_body[8][2] = { { -18.0, -7.0 },{ -18.0, 0.0 },{ -13.0, 0.0 },{ -10.0, 8.0 },{ 10.0, 8.0 },{ 13.0, 0.0 },{ 18.0, 0.0 },{ 18.0, -7.0 } };
GLfloat car2_front_window[4][2] = { { -10.0, 0.0 },{ -8.0, 6.0 },{ -2.0, 6.0 },{ -2.0, 0.0 } };
GLfloat car2_back_window[4][2] = { { 0.0, 0.0 },{ 0.0, 6.0 },{ 8.0, 6.0 },{ 10.0, 0.0 } };
GLfloat car2_front_wheel[8][2] = { { -11.0, -11.0 },{ -13.0, -8.0 },{ -13.0, -7.0 },{ -11.0, -4.0 },{ -7.0, -4.0 },{ -5.0, -7.0 },{ -5.0, -8.0 },{ -7.0, -11.0 } };
GLfloat car2_back_wheel[8][2] = { { 7.0, -11.0 },{ 5.0, -8.0 },{ 5.0, -7.0 },{ 7.0, -4.0 },{ 11.0, -4.0 },{ 13.0, -7.0 },{ 13.0, -8.0 },{ 11.0, -11.0 } };
GLfloat car2_light1[3][2] = { { -18.0, -1.0 },{ -15.0, -2.0 },{ -18.0, -3.0 } };
GLfloat car2_light2[3][2] = { { -18.0, -4.0 },{ -15.0, -5.0 },{ -18.0, -6.0 } };

GLfloat car2_color[7][3] = {
	{ 100 / 255.0f, 141 / 255.0f, 159 / 255.0f },
	{ 235 / 255.0f, 219 / 255.0f, 208 / 255.0f },
	{ 235 / 255.0f, 219 / 255.0f, 208 / 255.0f },
	{ 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
	{ 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f }
};

GLuint VBO_car2, VAO_car2;
void prepare_car2() {
	GLsizeiptr buffer_size = sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel)
		+ sizeof(car2_back_wheel) + sizeof(car2_light1) + sizeof(car2_light2);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_car2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car2);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(car2_body), car2_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body), sizeof(car2_front_window), car2_front_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window), sizeof(car2_back_window), car2_back_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window), sizeof(car2_front_wheel), car2_front_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel),
		sizeof(car2_back_wheel), car2_back_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel)
		+ sizeof(car2_back_wheel), sizeof(car2_light1), car2_light1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel)
		+ sizeof(car2_back_wheel) + sizeof(car2_light1), sizeof(car2_light2), car2_light2);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_car2);
	glBindVertexArray(VAO_car2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car2);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_car2() {
	glBindVertexArray(VAO_car2);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_FRONT_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BACK_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_FRONT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BACK_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 24, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_LIGHT1]);
	glDrawArrays(GL_TRIANGLE_FAN, 32, 3);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_LIGHT2]);
	glDrawArrays(GL_TRIANGLE_FAN, 35, 3);

	glBindVertexArray(0);
}

// hat
#define HAT_LEAF 0
#define HAT_BODY 1
#define HAT_STRIP 2
#define HAT_BOTTOM 3

GLfloat hat_leaf[4][2] = { { 3.0, 20.0 },{ 3.0, 28.0 },{ 9.0, 32.0 },{ 9.0, 24.0 } };
GLfloat hat_body[4][2] = { { -19.5, 2.0 },{ 19.5, 2.0 },{ 15.0, 20.0 },{ -15.0, 20.0 } };
GLfloat hat_strip[4][2] = { { -20.0, 0.0 },{ 20.0, 0.0 },{ 19.5, 2.0 },{ -19.5, 2.0 } };
GLfloat hat_bottom[4][2] = { { 25.0, 0.0 },{ -25.0, 0.0 },{ -25.0, -4.0 },{ 25.0, -4.0 } };

GLfloat hat_color[4][3] = {
	{ 167 / 255.0f, 255 / 255.0f, 55 / 255.0f },
{ 255 / 255.0f, 144 / 255.0f, 32 / 255.0f },
{ 255 / 255.0f, 40 / 255.0f, 33 / 255.0f },
{ 255 / 255.0f, 144 / 255.0f, 32 / 255.0f }
};

GLuint VBO_hat, VAO_hat;

void prepare_hat() {
	GLsizeiptr buffer_size = sizeof(hat_leaf) + sizeof(hat_body) + sizeof(hat_strip) + sizeof(hat_bottom);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_hat);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_hat);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(hat_leaf), hat_leaf);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hat_leaf), sizeof(hat_body), hat_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hat_leaf) + sizeof(hat_body), sizeof(hat_strip), hat_strip);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hat_leaf) + sizeof(hat_body) + sizeof(hat_strip), sizeof(hat_bottom), hat_bottom);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_hat);
	glBindVertexArray(VAO_hat);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_hat);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_hat() {
	glBindVertexArray(VAO_hat);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_LEAF]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_STRIP]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_BOTTOM]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glBindVertexArray(0);
}




// sword

#define SWORD_BODY 0
#define SWORD_BODY2 1
#define SWORD_HEAD 2
#define SWORD_HEAD2 3
#define SWORD_IN 4
#define SWORD_DOWN 5
#define SWORD_BODY_IN 6

GLfloat sword_body[4][2] = { { -6.0, 0.0 },{ -6.0, -4.0 },{ 6.0, -4.0 },{ 6.0, 0.0 } };
GLfloat sword_body2[4][2] = { { -2.0, -4.0 },{ -2.0, -6.0 } ,{ 2.0, -6.0 },{ 2.0, -4.0 } };
GLfloat sword_head[4][2] = { { -2.0, 0.0 },{ -2.0, 16.0 } ,{ 2.0, 16.0 },{ 2.0, 0.0 } };
GLfloat sword_head2[3][2] = { { -2.0, 16.0 },{ 0.0, 19.46 } ,{ 2.0, 16.0 } };
GLfloat sword_in[4][2] = { { -0.3, 0.7 },{ -0.3, 15.3 } ,{ 0.3, 15.3 },{ 0.3, 0.7 } };
GLfloat sword_down[4][2] = { { -2.0, -6.0 } ,{ 2.0, -6.0 },{ 4.0, -8.0 },{ -4.0, -8.0 } };
GLfloat sword_body_in[4][2] = { { 0.0, -1.0 } ,{ 1.0, -2.732 },{ 0.0, -4.464 },{ -1.0, -2.732 } };

GLfloat sword_color[7][3] = {
	{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
{ 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
{ 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 255 / 255.0f, 0 / 255.0f, 0 / 255.0f }
};

GLuint VBO_sword, VAO_sword;

void prepare_sword() {
	GLsizeiptr buffer_size = sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down) + sizeof(sword_body_in);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_sword);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sword_body), sword_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body), sizeof(sword_body2), sword_body2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2), sizeof(sword_head), sword_head);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head), sizeof(sword_head2), sword_head2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2), sizeof(sword_in), sword_in);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in), sizeof(sword_down), sword_down);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down), sizeof(sword_body_in), sword_body_in);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_sword);
	glBindVertexArray(VAO_sword);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_sword() {
	glBindVertexArray(VAO_sword);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD2]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_DOWN]);
	glDrawArrays(GL_TRIANGLE_FAN, 19, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 23, 4);

	glBindVertexArray(0);
}

void timer(int value) {
	timestamp = (timestamp + 1) % UINT_MAX;
	glutPostRedisplay();
	if (animation_mode)
		glutTimerFunc(10, timer, 0);
}

void display(void) {
	glm::mat4 ModelMatrix;

	glClear(GL_COLOR_BUFFER_BIT);

	ModelMatrix = glm::mat4(1.0f);
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_axes();

	int cloud_x = timestamp % 720;
	if (cloud_x >= 360) {
		cloud_x = 720 - cloud_x;
	}
	int cloud_clock = timestamp % 100;
	if (cloud_clock >= 50) {
		cloud_clock = 100 - cloud_clock;
	}
	
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 300.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f,-(float)cloud_clock*(2.0 / 3.0), 0.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3((float)cloud_x*(1.8/3.0), 0.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_cloud();

	int cloud_x1 = timestamp % 400;
	if (cloud_x1 >= 200) {
		cloud_x1 = 400 - cloud_x1;
	}
	int cloud_clock1 = timestamp % 120;
	if (cloud_clock1 >= 60) {
		cloud_clock1 = 120 - cloud_clock1;
	}

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 150.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.2f, 1.2f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -(float)cloud_clock1, 0.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-(float)cloud_x1*1.2, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_cloud();

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(300.0f, -150.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f, 3.0f, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_tree();

	int apple_clock = timestamp % 360;
	
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(260.0f, -105.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 2 * apple_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_apple();

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(340.0f, -105.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 3 * apple_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_apple();

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(300.0f, -65.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));
	ModelMatrix = glm::rotate(ModelMatrix, apple_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_apple();

	int person_clock = timestamp % 400;
	if (person_clock >= 200) {
		person_clock = 400 - person_clock;
	}
	
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(350.0f, -300.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-(float)person_clock, 0.0f, 0.0f));
	if (person_clock >= 150) {
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, -300.0f, 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, 90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_person();
	
	//ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-500.0f, 0.0f, 0.0f));
	int airplane_clock = timestamp % 360; // 0 <= airplane_clock <= 719 
	float AIRPLANE_ROTATION_RADIUS = 100.0f;
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 200.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, airplane_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-AIRPLANE_ROTATION_RADIUS, 0.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 2*airplane_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_airplane();


	

	int drink_clock = timestamp % 360;
	if (drink_clock >= 180)
	{
		drink_clock = 360 - drink_clock;
	}
	int water_clock = timestamp % 360;
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-200.0f, 300.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2+(float)drink_clock/50.0f,2 + (float)drink_clock / 50.0f,  1.0f));
	ModelMatrix = glm::rotate(ModelMatrix,  water_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_cocktail();

	
	


	int fb = 0;
	int car_clock = timestamp % 350;
	if (car_clock >= 175) {
		fb = 1;
		car_clock = 350 - car_clock;
	}
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-350.0f, -275.0f, 0.0f));
	if(fb)
	{
		
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 1.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3((float)car_clock, 0.0f, 0.0f));
	}
	else {
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(-1.0f, 1.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 1.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-(float)car_clock, 0.0f, 0.0f));
	}
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_car2();

	int hat_clock = timestamp % 400;
		if (hat_clock >= 200) {
			hat_clock = 400 - hat_clock;
		}

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(350.0f, -210.0f, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-(float)hat_clock, 0.0f, 0.0f));
	if (hat_clock >= 150)
	{
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, -310.0f, 0.0f));
		
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, (float)hat_clock, 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3((float)hat_clock/150.0f, (float)hat_clock/150.0f, 1.0f));
	}
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_hat();



	int sword_clock = timestamp % 350;
	if (sword_clock >= 175) {
		sword_clock = 350 - sword_clock;
	}
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f,0.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3((1 + ((float)sword_clock / 50.0f))*200.0f/350.0f   , 1 + ((float)sword_clock / 50.0f) * 200.0f / 350.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, (float)sword_clock * 200.0f / 350.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_sword();
	
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-200.0f, 0.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3((1 + ((float)sword_clock / 50.0f)) * 200.0f / 350.0f, 1 + ((float)sword_clock / 50.0f) * 200.0f / 350.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, (float)(sword_clock) * 200.0f / 350.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_sword();

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-300.0f, 0.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3((1 + ((float)sword_clock / 50.0f)) * 200.0f / 350.0f, 1 + ((float)sword_clock / 50.0f) * 200.0f / 350.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, (float)(sword_clock) * 200.0f / 350.0f, 0.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_sword();

	glFlush();
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}

void reshape(int width, int height) {
	win_width = width, win_height = height;

	glViewport(0, 0, win_width, win_height);
	ProjectionMatrix = glm::ortho(-win_width / 2.0, win_width / 2.0,
		-win_height / 2.0, win_height / 2.0, -1000.0, 1000.0);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	update_axes();
	update_line();

	glutPostRedisplay();
}

void cleanup(void) {
	glDeleteVertexArrays(1, &VAO_axes);
	glDeleteBuffers(1, &VBO_axes);

	glDeleteVertexArrays(1, &VAO_line);
	glDeleteBuffers(1, &VBO_line);

	glDeleteVertexArrays(1, &VAO_airplane);
	glDeleteBuffers(1, &VBO_airplane);

	// Delete others here too!!!
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutCloseFunc(cleanup);
	glutTimerFunc(10, timer, 0);
}

void prepare_shader_program(void) {
	ShaderInfo shader_info[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram, "u_primitive_color");
}

void initialize_OpenGL(void) {
	glEnable(GL_MULTISAMPLE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClearColor(0 / 255.0f, 255 / 255.0f, 255 / 255.0f, 1.0f);
	ViewMatrix = glm::mat4(1.0f);
}

void prepare_scene(void) {
	prepare_axes();
	prepare_line();
	prepare_airplane();
	
	prepare_car();
	prepare_cocktail();
	prepare_car2();
	prepare_hat();
	
	prepare_sword();
	prepare_tree();
	prepare_apple();
	prepare_person();
	prepare_cloud();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

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

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    HW2 by 20181632\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 1
void main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 HW2 coded by 20181632 ¹Ú¼ºÇö";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'ESC' "
	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize(800, 800);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}


