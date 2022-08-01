//
//  DrawScene.cpp
//
//  Written for CSE4170
//  Department of Computer Science and Engineering
//  Copyright © 2022 Sogang University. All rights reserved.
//

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <FreeImage/FreeImage.h>
#include "LoadScene.h"

// Begin of shader setup
#include "Shaders/LoadShaders.h"
#include "ShadingInfo.h"


#include <glm/gtc/matrix_inverse.hpp> // inverseTranspose, etc.
extern SCENE scene;

// for simple shaders
GLuint h_ShaderProgram_simple, h_ShaderProgram_TXPS, h_ShaderProgram_GRD; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

// for Phong Shading (Textured) shaders
#define NUMBER_OF_LIGHT_SUPPORTED 13
GLint loc_global_ambient_color;
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED+3];
loc_Material_Parameters loc_material;
GLint loc_ModelViewProjectionMatrix_TXPS, loc_ModelViewMatrix_TXPS, loc_ModelViewMatrixInvTrans_TXPS;
GLint loc_texture;
GLint loc_blind_effect;
GLint loc_screen_effect, loc_screen_frequency, loc_screen_width,loc_screen_effects;
GLint loc_ModelViewProjectionMatrix_PS, loc_ModelViewMatrix_PS, loc_ModelViewMatrixInvTrans_PS;
GLint loc_flag_texture_mapping;
GLint loc_flag_fog;
GLint loc_u_flag_blending, loc_u_fragment_alpha;


// for Gouraud Shading  shaders
GLint loc_global_ambient_color1;
loc_light_Parameters loc_light1[NUMBER_OF_LIGHT_SUPPORTED+3];
loc_Material_Parameters loc_material1;

GLint loc_ModelViewProjectionMatrix_GRD, loc_ModelViewMatrix_GRD, loc_ModelViewMatrixInvTrans_GRD;
GLint loc_texture1;
GLint loc_flag_texture_mapping1;
GLint loc_flag_fog1;
GLint loc_blind_effect1;

// include glm/*.hpp only if necessary
// #include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
// ViewProjectionMatrix = ProjectionMatrix * ViewMatrix
glm::mat4 ViewProjectionMatrix, ViewMatrix, ProjectionMatrix, ModelMatrix_tiger, ModelMatrix_tiger_sight;
// ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix
glm::mat4 Tiger_light;
glm::mat4 ModelViewProjectionMatrix; // This one is sent to vertex shader when ready.
glm::mat4 ModelViewMatrix;
glm::mat3 ModelViewMatrixInvTrans;

Light_Parameters light[5];

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_POSITION 0
#define LOC_VERTEX 0
#define LOC_NORMAL 1
#define LOC_TEXCOORD 2

int cur_frame_tiger, cur_frame_ben, cur_frame_wolf = 0;
float rotation_angle_tiger = 0.0f;
unsigned int timestamp = 0;
unsigned int tiger_timestamp = 0;
int animation_flag = 1;
int tiger_flag = 0;
int cur_frame_spider = 0;
int m_mode = 0;
int crtlpressed ,shiftpressed = 0;
int t,g = 0;//flag for tiger and tiger back camera
int up_down = 0;
int t_curcam;
int flag_shading = 1;
int flag_texture_map = 1;;

#define N_TEXTURES_USED 4
#define TEXTURE_ID_FLOOR 0
#define TEXTURE_ID_TIGER 1
#define TEXTURE_ID_ROBOT 2
#define TEXTURE_ID_SPIDER 3
GLuint texture_names[N_TEXTURES_USED];


void My_glTexImage2D_from_file(const char *filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP* tx_pixmap, * tx_pixmap_32;

	int width, height;
	GLvoid* data;

	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
	if (tx_bits_per_pixel == 32)
		tx_pixmap_32 = tx_pixmap;
	else {
		fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
		tx_pixmap_32 = FreeImage_ConvertTo32Bits(tx_pixmap);
	}

	width = FreeImage_GetWidth(tx_pixmap_32);
	height = FreeImage_GetHeight(tx_pixmap_32);
	data = FreeImage_GetBits(tx_pixmap_32);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap_32);
	if (tx_bits_per_pixel != 32)
		FreeImage_Unload(tx_pixmap);
}

/*********************************  START: camera *********************************/
typedef enum {
	CAMERA_1,
	CAMERA_2,
	CAMERA_3,
	CAMERA_4,
	CAMERA_5,
	CAMERA_6,
	CAMERA_T,
	CAMERA_G,
	NUM_CAMERAS
} CAMERA_INDEX;

typedef struct _Camera {
	float pos[3];
	float uaxis[3], vaxis[3], naxis[3];
	float fovy, aspect_ratio, near_c, far_c, zoom_factor;
	int move, rotation_axis;
} Camera;

Camera camera_info[NUM_CAMERAS];
Camera current_camera;

using glm::mat4;
void set_ViewMatrix_from_camera_frame(void) {
	ViewMatrix = glm::mat4(current_camera.uaxis[0], current_camera.vaxis[0], current_camera.naxis[0], 0.0f,
		current_camera.uaxis[1], current_camera.vaxis[1], current_camera.naxis[1], 0.0f,
		current_camera.uaxis[2], current_camera.vaxis[2], current_camera.naxis[2], 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	ViewMatrix = glm::translate(ViewMatrix, glm::vec3(-current_camera.pos[0], -current_camera.pos[1], -current_camera.pos[2]));
}

void set_current_camera(int camera_num) {
	Camera* pCamera = &camera_info[camera_num];
	glm::mat4 Matrix_camera_sight_inverse;
	ProjectionMatrix = glm::perspective(pCamera->fovy, pCamera->aspect_ratio, pCamera->near_c, pCamera->far_c);
	memcpy(&current_camera, pCamera, sizeof(Camera));
	t_curcam = camera_num;
	//fprintf(stdout, "%f, %f, %f\n", current_camera.pos[0], current_camera.pos[1], current_camera.pos[2]);
	
	set_ViewMatrix_from_camera_frame();
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void initialize_camera(void) {
	//CAMERA_1 : original view
	Camera* pCamera = &camera_info[CAMERA_1];
	for (int k = 0; k < 3; k++)
	{
		pCamera->pos[k] = scene.camera.e[k];
		pCamera->uaxis[k] = scene.camera.u[k];
		pCamera->vaxis[k] = scene.camera.v[k];
		pCamera->naxis[k] = scene.camera.n[k];
	}

	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;

	//CAMERA_2 : bronze statue view
	pCamera = &camera_info[CAMERA_2];
	pCamera->pos[0] = -593.047974f; pCamera->pos[1] = -3758.460938f; pCamera->pos[2] = 474.587830f;
	pCamera->uaxis[0] = 0.864306f; pCamera->uaxis[1] = -0.502877f; pCamera->uaxis[2] = 0.009328f;
	pCamera->vaxis[0] = 0.036087f; pCamera->vaxis[1] = 0.080500f; pCamera->vaxis[2] = 0.996094f;
	pCamera->naxis[0] = -0.501662f; pCamera->naxis[1] = -0.860599f; pCamera->naxis[2] = 0.087724f;
	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;

	//CAMERA_3 : bronze statue view
	pCamera = &camera_info[CAMERA_3];
	pCamera->pos[0] = -1.463161f; pCamera->pos[1] = 1720.545166f; pCamera->pos[2] = 683.703491f;
	pCamera->uaxis[0] = -0.999413f; pCamera->uaxis[1] = -0.032568f; pCamera->uaxis[2] = -0.010066f;
	pCamera->vaxis[0] = -0.011190f; pCamera->vaxis[1] = -0.034529f; pCamera->vaxis[2] = 0.999328f;
	pCamera->naxis[0] = -0.032200f; pCamera->naxis[1] = 0.998855f; pCamera->naxis[2] = -0.034872f;
	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;
	
	//CAMERA_4 : top view
	pCamera = &camera_info[CAMERA_4];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = 0.0f; pCamera->pos[2] = 18300.0f;
	pCamera->uaxis[0] = 1.0f; pCamera->uaxis[1] = 0.0f; pCamera->uaxis[2] = 0.0f;
	pCamera->vaxis[0] = 0.0f; pCamera->vaxis[1] = 1.0f; pCamera->vaxis[2] = 0.0f;
	pCamera->naxis[0] = 0.0f; pCamera->naxis[1] = 0.0f; pCamera->naxis[2] = 1.0f;
	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;

	/*//CAMERA_5 : front view
	pCamera = &camera_info[CAMERA_5];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = 11700.0f; pCamera->pos[2] = 0.0f;
	pCamera->uaxis[0] = 1.0f; pCamera->uaxis[1] = 0.0f; pCamera->uaxis[2] = 0.0f;
	pCamera->vaxis[0] = 0.0f; pCamera->vaxis[1] = 0.0f; pCamera->vaxis[2] = 1.0f;
	pCamera->naxis[0] = 0.0f; pCamera->naxis[1] = 1.0f; pCamera->naxis[2] = 0.0f;
	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;

	//CAMERA_6 : side view
	pCamera = &camera_info[CAMERA_6];
	pCamera->pos[0] = 14600.0f; pCamera->pos[1] = 0.0f; pCamera->pos[2] = 0.0f;
	pCamera->uaxis[0] = 0.0f; pCamera->uaxis[1] = 1.0f; pCamera->uaxis[2] = 0.0f;
	pCamera->vaxis[0] = 0.0f; pCamera->vaxis[1] = 0.0f; pCamera->vaxis[2] = 1.0f;
	pCamera->naxis[0] = 1.0f; pCamera->naxis[1] = 0.0f; pCamera->naxis[2] = 0.0f;
	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;
*/
	//CAMERA_T : Tiger camera
	pCamera = &camera_info[CAMERA_T];
	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;

	//CAMERA_G : Tiger back camera
	pCamera = &camera_info[CAMERA_G];
	pCamera->move = 0;
	pCamera->zoom_factor = 1.0f;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f;
	
	
	set_current_camera(CAMERA_2);
}
enum axes { X_AXIS, Y_AXIS, Z_AXIS, NORMAL };
static int flag_translation_axis;

#define CAM_RSPEED 0.1f
void renew_cam_orientation_rotation_around_v_axis(int angle) {
	// let's get a help from glm
	glm::mat3 RotationMatrix;
	glm::vec3 direction;

	RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle,
		glm::vec3(current_camera.vaxis[0], current_camera.vaxis[1], current_camera.vaxis[2])));

	direction = RotationMatrix * glm::vec3(current_camera.uaxis[0], current_camera.uaxis[1], current_camera.uaxis[2]);
	current_camera.uaxis[0] = direction.x; current_camera.uaxis[1] = direction.y; current_camera.uaxis[2] = direction.z;
	direction = RotationMatrix * glm::vec3(current_camera.naxis[0], current_camera.naxis[1], current_camera.naxis[2]);
	current_camera.naxis[0] = direction.x; current_camera.naxis[1] = direction.y; current_camera.naxis[2] = direction.z;
}
void renew_cam_orientation_rotation_around_u_axis(int angle) {
	// let's get a help from glm
	glm::mat3 RotationMatrix;
	glm::vec3 direction;

	RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle,
		glm::vec3(current_camera.uaxis[0], current_camera.uaxis[1], current_camera.uaxis[2])));

	direction = RotationMatrix * glm::vec3(current_camera.vaxis[0], current_camera.vaxis[1], current_camera.vaxis[2]);
	current_camera.vaxis[0] = direction.x; current_camera.vaxis[1] = direction.y; current_camera.vaxis[2] = direction.z;
	direction = RotationMatrix * glm::vec3(current_camera.naxis[0], current_camera.naxis[1], current_camera.naxis[2]);
	current_camera.naxis[0] = direction.x; current_camera.naxis[1] = direction.y; current_camera.naxis[2] = direction.z;
}

void renew_cam_orientation_rotation_around_n_axis(int angle) {
	// let's get a help from glm
	glm::mat3 RotationMatrix;
	glm::vec3 direction;

	RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle,
		glm::vec3(current_camera.naxis[0], current_camera.naxis[1], current_camera.naxis[2])));

	direction = RotationMatrix * glm::vec3(current_camera.vaxis[0], current_camera.vaxis[1], current_camera.vaxis[2]);
	current_camera.vaxis[0] = direction.x; current_camera.vaxis[1] = direction.y; current_camera.vaxis[2] = direction.z;
	direction = RotationMatrix * glm::vec3(current_camera.uaxis[0], current_camera.uaxis[1], current_camera.uaxis[2]);
	current_camera.uaxis[0] = direction.x; current_camera.uaxis[1] = direction.y; current_camera.uaxis[2] = direction.z;
}
void tiger_sight_camera(void) {
	
	
		ModelMatrix_tiger_sight = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -88.0f, 62.0f));
		ModelMatrix_tiger_sight = glm::rotate(ModelMatrix_tiger_sight, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		ModelMatrix_tiger_sight = glm::rotate(ModelMatrix_tiger_sight, 180 * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
}

void tiger_back_sight_camera(void) {
	
	ModelMatrix_tiger_sight = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 88.0f, 80.0f));
	ModelMatrix_tiger_sight = glm::rotate(ModelMatrix_tiger_sight, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelMatrix_tiger_sight = glm::rotate(ModelMatrix_tiger_sight, 180 * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));

}
void set_ViewMatrix_for_Tiger(void) {
	glm::mat4 Matrix_camera_sight_inverse;

	Matrix_camera_sight_inverse = ModelMatrix_tiger * ModelMatrix_tiger_sight;
	ViewMatrix = glm::affineInverse(Matrix_camera_sight_inverse);
}

void set_tiger_camera(int camera_num) {
	glm::mat4 Matrix_camera_sight_inverse;
	Camera* pCamera = &camera_info[camera_num];
	memcpy(&current_camera, pCamera, sizeof(Camera));
	Matrix_camera_sight_inverse = ModelMatrix_tiger * ModelMatrix_tiger_sight;
	ViewMatrix = glm::affineInverse(Matrix_camera_sight_inverse);

	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	
}
/*********************************  END: camera *********************************/

/******************************  START: shader setup ****************************/
// Begin of Callback function definitions
void prepare_shader_program(void) {
	char string[256];
	char string1[256];

	ShaderInfo shader_info[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};

	ShaderInfo shader_info_TXPS[3] = {
	{ GL_VERTEX_SHADER, "Shaders/Phong_Tx.vert" },
	{ GL_FRAGMENT_SHADER, "Shaders/Phong_Tx.frag" },
	{ GL_NONE, NULL }
	};

	ShaderInfo shader_info_GRD[3] = {
	{ GL_VERTEX_SHADER, "Shaders/Gouraud.vert" },
	{ GL_FRAGMENT_SHADER, "Shaders/Gouraud.frag" },
	{ GL_NONE, NULL }
	};

	h_ShaderProgram_simple = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram_simple);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");

	h_ShaderProgram_TXPS = LoadShaders(shader_info_TXPS);
	loc_ModelViewProjectionMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrixInvTrans");

	loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_global_ambient_color");

	for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED+3; i++) {
		sprintf(string, "u_light[%d].light_on", i);
		loc_light[i].light_on = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].position", i);
		loc_light[i].position = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].ambient_color", i);
		loc_light[i].ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].diffuse_color", i);
		loc_light[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].specular_color", i);
		loc_light[i].specular_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].spot_direction", i);
		loc_light[i].spot_direction = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].spot_exponent", i);
		loc_light[i].spot_exponent = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].spot_cutoff_angle", i);
		loc_light[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].light_attenuation_factors", i);
		loc_light[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_TXPS, string);
	}

	loc_material.ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.ambient_color");
	loc_material.diffuse_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.diffuse_color");
	loc_material.specular_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.specular_color");
	loc_material.emissive_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.emissive_color");
	loc_material.specular_exponent = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.specular_exponent");

	loc_screen_effect = glGetUniformLocation(h_ShaderProgram_TXPS, "screen_effect");
	loc_screen_frequency = glGetUniformLocation(h_ShaderProgram_TXPS, "screen_frequency");
	loc_screen_width = glGetUniformLocation(h_ShaderProgram_TXPS, "screen_width");
	loc_u_flag_blending = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_blending");
	loc_blind_effect = glGetUniformLocation(h_ShaderProgram_TXPS, "u_blind_effect");
	loc_u_fragment_alpha = glGetUniformLocation(h_ShaderProgram_TXPS, "u_fragment_alpha");
	loc_texture = glGetUniformLocation(h_ShaderProgram_TXPS, "u_base_texture");
	loc_flag_texture_mapping = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_texture_mapping");
	loc_flag_fog = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_fog");

	h_ShaderProgram_GRD = LoadShaders(shader_info_GRD);
	loc_ModelViewProjectionMatrix_GRD = glGetUniformLocation(h_ShaderProgram_GRD, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_GRD = glGetUniformLocation(h_ShaderProgram_GRD, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_GRD = glGetUniformLocation(h_ShaderProgram_GRD, "u_ModelViewMatrixInvTrans");


	loc_global_ambient_color1 = glGetUniformLocation(h_ShaderProgram_GRD, "u_global_ambient_color");

	for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED+3; i++) {
		sprintf(string1, "u_light[%d].light_on", i);
		loc_light1[i].light_on = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].position", i);
		loc_light1[i].position = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].ambient_color", i);
		loc_light1[i].ambient_color = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].diffuse_color", i);
		loc_light1[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].specular_color", i);
		loc_light1[i].specular_color = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].spot_direction", i);
		loc_light1[i].spot_direction = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].spot_exponent", i);
		loc_light1[i].spot_exponent = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].spot_cutoff_angle", i);
		loc_light1[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_GRD, string1);
		sprintf(string1, "u_light[%d].light_attenuation_factors", i);
		loc_light1[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_GRD, string1);
	}

	loc_material1.ambient_color = glGetUniformLocation(h_ShaderProgram_GRD, "u_material.ambient_color");
	loc_material1.diffuse_color = glGetUniformLocation(h_ShaderProgram_GRD, "u_material.diffuse_color");
	loc_material1.specular_color = glGetUniformLocation(h_ShaderProgram_GRD, "u_material.specular_color");
	loc_material1.emissive_color = glGetUniformLocation(h_ShaderProgram_GRD, "u_material.emissive_color");
	loc_material1.specular_exponent = glGetUniformLocation(h_ShaderProgram_GRD, "u_material.specular_exponent");

	loc_blind_effect1 = glGetUniformLocation(h_ShaderProgram_GRD, "u_blind_effect");
	loc_texture1 = glGetUniformLocation(h_ShaderProgram_GRD, "u_base_texture");
	loc_flag_texture_mapping1 = glGetUniformLocation(h_ShaderProgram_GRD, "u_flag_texture_mapping");
	loc_flag_fog1 = glGetUniformLocation(h_ShaderProgram_GRD, "u_flag_fog");
}
/*******************************  END: shder setup ******************************/

/****************************  START: geometry setup ****************************/
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))
#define INDEX_VERTEX_POSITION	0
#define INDEX_NORMAL			1
#define INDEX_TEX_COORD			2

bool b_draw_grid = false;

//axes
GLuint axes_VBO, axes_VAO;
GLfloat axes_vertices[6][3] = {
	{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
};
GLfloat axes_color[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

void prepare_axes(void) {
	// Initialize vertex buffer object.
	glGenBuffers(1, &axes_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes_vertices), &axes_vertices[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &axes_VAO);
	glBindVertexArray(axes_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	fprintf(stdout, " * Loaded axes into graphics memory.\n");
}

void draw_axes(void) {
	if (!b_draw_grid)
		return;

	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(8000.0f, 8000.0f, 8000.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(2.0f);
	glBindVertexArray(axes_VAO);
	glUniform3fv(loc_primitive_color, 1, axes_color[0]);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[1]);
	glDrawArrays(GL_LINES, 2, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[2]);
	glDrawArrays(GL_LINES, 4, 2);
	glBindVertexArray(0);
	glLineWidth(1.0f);
	glUseProgram(0);
}

//grid
#define GRID_LENGTH			(100)
#define NUM_GRID_VETICES	((2 * GRID_LENGTH + 1) * 4)
GLuint grid_VBO, grid_VAO;
GLfloat grid_vertices[NUM_GRID_VETICES][3];
GLfloat grid_color[3] = { 0.5f, 0.5f, 0.5f };

void prepare_grid(void) {

	//set grid vertices
	int vertex_idx = 0;
	for (int x_idx = -GRID_LENGTH; x_idx <= GRID_LENGTH; x_idx++)
	{
		grid_vertices[vertex_idx][0] = x_idx;
		grid_vertices[vertex_idx][1] = -GRID_LENGTH;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;

		grid_vertices[vertex_idx][0] = x_idx;
		grid_vertices[vertex_idx][1] = GRID_LENGTH;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;
	}

	for (int y_idx = -GRID_LENGTH; y_idx <= GRID_LENGTH; y_idx++)
	{
		grid_vertices[vertex_idx][0] = -GRID_LENGTH;
		grid_vertices[vertex_idx][1] = y_idx;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;

		grid_vertices[vertex_idx][0] = GRID_LENGTH;
		grid_vertices[vertex_idx][1] = y_idx;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;
	}

	// Initialize vertex buffer object.
	glGenBuffers(1, &grid_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, grid_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(grid_vertices), &grid_vertices[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &grid_VAO);
	glBindVertexArray(grid_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, grid_VAO);
	glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	fprintf(stdout, " * Loaded grid into graphics memory.\n");
}

void draw_grid(void) {
	if (!b_draw_grid)
		return;

	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(1.0f);
	glBindVertexArray(grid_VAO);
	glUniform3fv(loc_primitive_color, 1, grid_color);
	glDrawArrays(GL_LINES, 0, NUM_GRID_VETICES);
	glBindVertexArray(0);
	glLineWidth(1.0f);
	glUseProgram(0);
}

//sun_temple
GLuint* sun_temple_VBO;
GLuint* sun_temple_VAO;
int* sun_temple_n_triangles;
int* sun_temple_vertex_offset;
GLfloat** sun_temple_vertices;
GLuint* sun_temple_texture_names;

int flag_fog;
bool* flag_texture_mapping;

void initialize_lights(void) { // follow OpenGL conventions for initialization
	int i;


	light[1].light_on = 1;
	light[1].position[0] = 0.0f; light[1].position[1] = 0.0f; // spot light position in WC
	light[1].position[2] = 0.0f; light[1].position[3] = 1.0f;

	//light[0].position[0] = -593.047974f; light[0].position[1] = -3758.460938f; // spot light position in WC
	//light[0].position[2] = 474.587830f; light[0].position[3] = 0.0f;

	glUseProgram(h_ShaderProgram_TXPS);

	glUniform4f(loc_global_ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);

	for (i = 0; i < scene.n_lights; i++) {
		glUniform1i(loc_light[i].light_on, 1);
		glUniform4f(loc_light[i].position,
			scene.light_list[i].pos[0],
			scene.light_list[i].pos[1],
			scene.light_list[i].pos[2],
			1.0f);
		

		glUniform4f(loc_light[i].ambient_color, 0.13f, 0.13f, 0.13f, 1.0f);
		glUniform4f(loc_light[i].diffuse_color, 0.5f, 0.5f, 0.5f, 1.0f);
		glUniform4f(loc_light[i].specular_color, 0.8f, 0.8f, 0.8f, 1.0f);
		glUniform3f(loc_light[i].spot_direction, 0.0f, 0.0f, -1.0f);
		glUniform1f(loc_light[i].spot_exponent, 0.0f); // [0.0, 128.0]
		glUniform1f(loc_light[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
		glUniform4f(loc_light[i].light_attenuation_factors, 20.0f, 0.0f, 0.0f, 1.0f); // .w != 0.0f for no ligth attenuation
	} //- 593.047974f, -3758.460938f, 474.587830f, 1.0f)
	
/*---------------------------------------------------------------------------------------------------------------------------------------*/
	glUniform1i(loc_light[13].light_on, 0);
	glm::vec4 position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
		light[1].position[2], light[1].position[3]);
	glUniform4fv(loc_light[13].position,1, &position_EC[0]);

	glUniform4f(loc_light[13].ambient_color, 0.5f, 0.5f, 0.5f, 1.0f);
	glUniform4f(loc_light[13].diffuse_color, 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(loc_light[13].specular_color, 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform3f(loc_light[13].spot_direction, 0.0f, 0.0f, -1.0f);
	glUniform1f(loc_light[13].spot_exponent, 50.0f); // [0.0, 128.0]
	glUniform1f(loc_light[13].spot_cutoff_angle, 30.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
	glUniform4f(loc_light[13].light_attenuation_factors, 1.0f, 0.0f, 0.0f, 1.0f); // .w != 0.0f for no ligth attenuation
	
	glUniform1i(loc_blind_effect, 0);
	
	

	glUseProgram(0);

	

	glUseProgram(h_ShaderProgram_GRD);

	glUniform4f(loc_global_ambient_color1, 0.2f, 0.2f, 0.2f, 1.0f);

	for (i = 0; i < scene.n_lights; i++) {
		glUniform1i(loc_light1[i].light_on, 1);
		glUniform4f(loc_light1[i].position,
			scene.light_list[i].pos[0],
			scene.light_list[i].pos[1],
			scene.light_list[i].pos[2],
			1.0f);

		glUniform4f(loc_light1[i].ambient_color, 0.13f, 0.13f, 0.13f, 1.0f);
		glUniform4f(loc_light1[i].diffuse_color, 0.5f, 0.5f, 0.5f, 1.0f);
		glUniform4f(loc_light1[i].specular_color, 0.8f, 0.8f, 0.8f, 1.0f);
		glUniform3f(loc_light1[i].spot_direction, 0.0f, 0.0f, -1.0f);
		glUniform1f(loc_light1[i].spot_exponent, 0.0f); // [0.0, 128.0]
		glUniform1f(loc_light1[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
		glUniform4f(loc_light1[i].light_attenuation_factors, 20.0f, 0.0f, 0.0f, 1.0f); // .w != 0.0f for no ligth attenuation
	}
	glUniform1i(loc_light1[13].light_on, 0);
	glUniform4fv(loc_light1[13].position, 1, &position_EC[0]);

	glUniform4f(loc_light1[13].ambient_color, 0.5f, 0.5f, 0.5f, 1.0f);
	glUniform4f(loc_light1[13].diffuse_color, 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(loc_light1[13].specular_color, 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform3f(loc_light1[13].spot_direction, 0.0f, 0.0f, -1.0f);
	glUniform1f(loc_light1[13].spot_exponent, 50.0f); // [0.0, 128.0]
	glUniform1f(loc_light1[13].spot_cutoff_angle, 30.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
	glUniform4f(loc_light1[13].light_attenuation_factors, 1.0f, 0.0f, 0.0f, 1.0f); // .w != 0.0f for no ligth attenuation

	glUseProgram(0);
}
void set_light(void)
{
	
}
void initialize_flags(void) {
	flag_fog = 0;

	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_fog, flag_fog);
	glUniform1i(loc_flag_texture_mapping, flag_texture_map);
	glUseProgram(0);
}

bool readTexImage2D_from_file(char* filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP* tx_pixmap, * tx_pixmap_32;

	int width, height;
	GLvoid* data;

	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	if (tx_pixmap == NULL)
		return false;
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	//fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
	if (tx_bits_per_pixel == 32)
		tx_pixmap_32 = tx_pixmap;
	else {
		//fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
		tx_pixmap_32 = FreeImage_ConvertTo32Bits(tx_pixmap);
	}

	width = FreeImage_GetWidth(tx_pixmap_32);
	height = FreeImage_GetHeight(tx_pixmap_32);
	data = FreeImage_GetBits(tx_pixmap_32);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	//fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap_32);
	if (tx_bits_per_pixel != 32)
		FreeImage_Unload(tx_pixmap);

	return true;
}

void prepare_sun_temple(void) {
	int n_bytes_per_vertex, n_bytes_per_triangle;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	// VBO, VAO malloc
	sun_temple_VBO = (GLuint*)malloc(sizeof(GLuint) * scene.n_materials);
	sun_temple_VAO = (GLuint*)malloc(sizeof(GLuint) * scene.n_materials);

	sun_temple_n_triangles = (int*)malloc(sizeof(int) * scene.n_materials);
	sun_temple_vertex_offset = (int*)malloc(sizeof(int) * scene.n_materials);

	flag_texture_mapping = (bool*)malloc(sizeof(bool) * scene.n_textures);

	// vertices
	sun_temple_vertices = (GLfloat**)malloc(sizeof(GLfloat*) * scene.n_materials);

	for (int materialIdx = 0; materialIdx < scene.n_materials; materialIdx++) {
		MATERIAL* pMaterial = &(scene.material_list[materialIdx]);
		GEOMETRY_TRIANGULAR_MESH* tm = &(pMaterial->geometry.tm);

		// vertex
		sun_temple_vertices[materialIdx] = (GLfloat*)malloc(sizeof(GLfloat) * 8 * tm->n_triangle * 3);

		int vertexIdx = 0;
		for (int triIdx = 0; triIdx < tm->n_triangle; triIdx++) {
			TRIANGLE tri = tm->triangle_list[triIdx];
			for (int triVertex = 0; triVertex < 3; triVertex++) {
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].x;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].y;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].z;

				sun_temple_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].x;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].y;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].z;

				sun_temple_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].u;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].v;
			}
		}

		// # of triangles
		sun_temple_n_triangles[materialIdx] = tm->n_triangle;

		if (materialIdx == 0)
			sun_temple_vertex_offset[materialIdx] = 0;
		else
			sun_temple_vertex_offset[materialIdx] = sun_temple_vertex_offset[materialIdx - 1] + 3 * sun_temple_n_triangles[materialIdx - 1];

		glGenBuffers(1, &sun_temple_VBO[materialIdx]);

		glBindBuffer(GL_ARRAY_BUFFER, sun_temple_VBO[materialIdx]);
		glBufferData(GL_ARRAY_BUFFER, sun_temple_n_triangles[materialIdx] * 3 * n_bytes_per_vertex,
			sun_temple_vertices[materialIdx], GL_STATIC_DRAW);

		// As the geometry data exists now in graphics memory, ...
		free(sun_temple_vertices[materialIdx]);

		// Initialize vertex array object.
		glGenVertexArrays(1, &sun_temple_VAO[materialIdx]);
		glBindVertexArray(sun_temple_VAO[materialIdx]);

		glBindBuffer(GL_ARRAY_BUFFER, sun_temple_VBO[materialIdx]);
		glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
		glEnableVertexAttribArray(INDEX_VERTEX_POSITION);
		glVertexAttribPointer(INDEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
		glEnableVertexAttribArray(INDEX_NORMAL);
		glVertexAttribPointer(INDEX_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(6 * sizeof(float)));
		glEnableVertexAttribArray(INDEX_TEX_COORD);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		if ((materialIdx > 0) && (materialIdx % 100 == 0))
			fprintf(stdout, " * Loaded %d sun temple materials into graphics memory.\n", materialIdx / 100 * 100);
	}
	fprintf(stdout, " * Loaded %d sun temple materials into graphics memory.\n", scene.n_materials);

	// textures
	sun_temple_texture_names = (GLuint*)malloc(sizeof(GLuint) * scene.n_textures);
	glGenTextures(scene.n_textures, sun_temple_texture_names);

	for (int texId = 0; texId < scene.n_textures; texId++) {
		glActiveTexture(GL_TEXTURE0 + texId);
		glBindTexture(GL_TEXTURE_2D, sun_temple_texture_names[texId]);

		bool bReturn = readTexImage2D_from_file(scene.texture_file_name[texId]);

		if (bReturn) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			flag_texture_mapping[texId] = true;
		}
		else {
			flag_texture_mapping[texId] = false;
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	fprintf(stdout, " * Loaded sun temple textures into graphics memory.\n\n");
	
	free(sun_temple_vertices);
}



// TO DO
void prepare_objects(void) {
	/* fill your code here */
}

void draw_objects(void) {
	/* fill your code here */
}
#define N_TIGER_FRAMES 12
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat* tiger_vertices[N_TIGER_FRAMES];

Material_Parameters material_tiger;

int read_geometry(GLfloat** object, int bytes_per_primitive, char* filename) {
	int n_triangles;
	FILE* fp;

	// fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open the object file %s ...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);
	*object = (float*)malloc(n_triangles * bytes_per_primitive);
	if (*object == NULL) {
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	fread(*object, bytes_per_primitive, n_triangles, fp);
	// fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}

void set_material_tiger(void) {
	// assume ShaderProgram_TXPS is used
	glUniform4fv(loc_material.ambient_color, 1, material_tiger.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_tiger.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_tiger.specular_color);
	glUniform1f(loc_material.specular_exponent, material_tiger.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_tiger.emissive_color);
}

void set_material_tiger1(void) {
	// assume ShaderProgram_GRD is used
	glUniform4fv(loc_material1.ambient_color, 1, material_tiger.ambient_color);
	glUniform4fv(loc_material1.diffuse_color, 1, material_tiger.diffuse_color);
	glUniform4fv(loc_material1.specular_color, 1, material_tiger.specular_color);
	glUniform1f(loc_material1.specular_exponent, material_tiger.specular_exponent);
	glUniform4fv(loc_material1.emissive_color, 1, material_tiger.emissive_color);
}

void prepare_tiger(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// Assume all geometry files are effective.
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// Initialize vertex buffer object.
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
			tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// As the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// Initialize vertex array object.
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	

}


int flag_cull_face = 0;



// spider object
#define N_SPIDER_FRAMES 16
GLuint spider_VBO, spider_VAO;
int spider_n_triangles[N_SPIDER_FRAMES];
int spider_vertex_offset[N_SPIDER_FRAMES];
GLfloat* spider_vertices[N_SPIDER_FRAMES];


void prepare_spider(void) {

	int i, n_bytes_per_vertex, n_bytes_per_triangle, spider_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_SPIDER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/spider/spider_vnt_%d%d.geom", i / 10, i % 10);
		spider_n_triangles[i] = read_geometry(&spider_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		spider_n_total_triangles += spider_n_triangles[i];

		if (i == 0)
			spider_vertex_offset[i] = 0;
		else
			spider_vertex_offset[i] = spider_vertex_offset[i - 1] + 3 * spider_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &spider_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glBufferData(GL_ARRAY_BUFFER, spider_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_SPIDER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, spider_vertex_offset[i] * n_bytes_per_vertex,
			spider_n_triangles[i] * n_bytes_per_triangle, spider_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_SPIDER_FRAMES; i++)
		free(spider_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &spider_VAO);
	glBindVertexArray(spider_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_SPIDER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_SPIDER]);

	My_glTexImage2D_from_file("Data/spider.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	
}



// ben object
#define N_BEN_FRAMES 30
GLuint ben_VBO, ben_VAO;
int ben_n_triangles[N_BEN_FRAMES];
int ben_vertex_offset[N_BEN_FRAMES];
GLfloat* ben_vertices[N_BEN_FRAMES];

Material_Parameters material_ben;

void prepare_ben(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, ben_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_BEN_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/ben/ben_vn%d%d.geom", i / 10, i % 10);
		ben_n_triangles[i] = read_geometry(&ben_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		ben_n_total_triangles += ben_n_triangles[i];

		if (i == 0)
			ben_vertex_offset[i] = 0;
		else
			ben_vertex_offset[i] = ben_vertex_offset[i - 1] + 3 * ben_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &ben_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glBufferData(GL_ARRAY_BUFFER, ben_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_BEN_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, ben_vertex_offset[i] * n_bytes_per_vertex,
			ben_n_triangles[i] * n_bytes_per_triangle, ben_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_BEN_FRAMES; i++)
		free(ben_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &ben_VAO);
	glBindVertexArray(ben_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	

	
}



// ironman object
GLuint ironman_VBO, ironman_VAO;
int ironman_n_triangles;
GLfloat* ironman_vertices;

Material_Parameters material_ironman;


void set_material_ironman(void) {
	// assume ShaderProgram_GD is used
	glUniform4fv(loc_material1.ambient_color, 1, material_ironman.ambient_color);
	glUniform4fv(loc_material1.diffuse_color, 1, material_ironman.diffuse_color);
	glUniform4fv(loc_material1.specular_color, 1, material_ironman.specular_color);
	glUniform1f(loc_material1.specular_exponent, material_ironman.specular_exponent);
	glUniform4fv(loc_material1.emissive_color, 1, material_ironman.emissive_color);
}

void set_material_ironman1(void) {
	// assume ShaderProgram_GD is used
	glUniform4fv(loc_material.ambient_color, 1, material_ironman.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_ironman.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_ironman.specular_color);
	glUniform1f(loc_material.specular_exponent, material_ironman.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_ironman.emissive_color);
}

void prepare_ironman(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, ironman_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/ironman_vnt.geom");
	ironman_n_triangles = read_geometry(&ironman_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	ironman_n_total_triangles += ironman_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &ironman_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
	glBufferData(GL_ARRAY_BUFFER, ironman_n_total_triangles * 3 * n_bytes_per_vertex, ironman_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(ironman_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &ironman_VAO);
	glBindVertexArray(ironman_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_ironman.ambient_color[0] = 0.19225f;
	material_ironman.ambient_color[1] = 0.19225f;
	material_ironman.ambient_color[2] = 0.19225f;
	material_ironman.ambient_color[3] = 1.0f;

	material_ironman.diffuse_color[0] = 0.50754f;
	material_ironman.diffuse_color[1] = 0.50754f;
	material_ironman.diffuse_color[2] = 0.50754f;
	material_ironman.diffuse_color[3] = 1.0f;

	material_ironman.specular_color[0] = 0.508273f;
	material_ironman.specular_color[1] = 0.508273f;
	material_ironman.specular_color[2] = 0.508373f;
	material_ironman.specular_color[3] = 1.0f;

	material_ironman.specular_exponent = 51.2f;

	material_ironman.emissive_color[0] = 0.3f;
	material_ironman.emissive_color[1] = 0.05f;
	material_ironman.emissive_color[2] = 0.05f;
	material_ironman.emissive_color[3] = 1.0f;

	
	
}



// cow object
GLuint cow_VBO, cow_VAO;
int cow_n_triangles;
GLfloat* cow_vertices;

Material_Parameters material_cow;
float tiger_alpha;

void prepare_cow(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, cow_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/cow_vn.geom");
	cow_n_triangles = read_geometry(&cow_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	cow_n_total_triangles += cow_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &cow_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, cow_VBO);
	glBufferData(GL_ARRAY_BUFFER, cow_n_total_triangles * 3 * n_bytes_per_vertex, cow_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(cow_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &cow_VAO);
	glBindVertexArray(cow_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, cow_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_tiger.ambient_color[0] = 0.24725f;
	material_tiger.ambient_color[1] = 0.1995f;
	material_tiger.ambient_color[2] = 0.0745f;
	material_tiger.ambient_color[3] = 1.0f;

	material_tiger.diffuse_color[0] = 0.75164f;
	material_tiger.diffuse_color[1] = 0.60648f;
	material_tiger.diffuse_color[2] = 0.22648f;
	material_tiger.diffuse_color[3] = 1.0f;

	material_tiger.specular_color[0] = 0.628281f;
	material_tiger.specular_color[1] = 0.555802f;
	material_tiger.specular_color[2] = 0.366065f;
	material_tiger.specular_color[3] = 1.0f;

	material_tiger.specular_exponent = 51.2f;

	material_tiger.emissive_color[0] = 0.1f;
	material_tiger.emissive_color[1] = 0.1f;
	material_tiger.emissive_color[2] = 0.0f;
	material_tiger.emissive_color[3] = 1.0f;

	tiger_alpha = 1.0f;
}

// optimus object
GLuint optimus_VBO, optimus_VAO;
int optimus_n_triangles;
GLfloat* optimus_vertices;

Material_Parameters material_optimus;


void prepare_optimus(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, optimus_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/optimus_vnt.geom");
	optimus_n_triangles = read_geometry(&optimus_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	optimus_n_total_triangles += optimus_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &optimus_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glBufferData(GL_ARRAY_BUFFER, optimus_n_total_triangles * 3 * n_bytes_per_vertex, optimus_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(optimus_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &optimus_VAO);
	glBindVertexArray(optimus_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_ROBOT);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_ROBOT]);

	My_glTexImage2D_from_file("Data/robot.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	
}

// bike object
GLuint bike_VBO, bike_VAO;
int bike_n_triangles;
GLfloat* bike_vertices;

Material_Parameters material_bike;
void prepare_bike(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, bike_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/bike_vnt.geom");
	bike_n_triangles = read_geometry(&bike_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	bike_n_total_triangles += bike_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &bike_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
	glBufferData(GL_ARRAY_BUFFER, bike_n_total_triangles * 3 * n_bytes_per_vertex, bike_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(bike_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &bike_VAO);
	glBindVertexArray(bike_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


}
// dragon object
GLuint dragon_VBO, dragon_VAO;
int dragon_n_triangles;
GLfloat* dragon_vertices;

Material_Parameters material_dragon;

void prepare_dragon(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, dragon_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/dragon_vnt.geom");
	dragon_n_triangles = read_geometry(&dragon_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	dragon_n_total_triangles += dragon_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &dragon_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glBufferData(GL_ARRAY_BUFFER, dragon_n_total_triangles * 3 * n_bytes_per_vertex, dragon_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(dragon_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &dragon_VAO);
	glBindVertexArray(dragon_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}

GLuint cube_VBO, cube_VAO;
GLfloat cube_vertices[72][3] = { // vertices enumerated counterclockwise
	{ -1.0f, -1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, -1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f },
	{ -1.0f, 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f },
	{ -1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f },
	{ 1.0f, -1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, { -1.0f, -1.0f, -1.0f }, { 0.0f, -1.0f, 0.0f },
	{ 1.0f, -1.0f, -1.0f }, { 0.0f, -1.0f, 0.0f },
	{ 1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f },
	{ -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, -1.0f },
	{ -1.0f, -1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { -1.0f, 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f },
	{ -1.0f, 1.0f, -1.0f }, { -1.0f, 0.0f, 0.0f },
	{ 1.0f, -1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, { -1.0f, -1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f },
	{ -1.0f, -1.0f, -1.0f }, { 0.0f, -1.0f, 0.0f },
	{ -1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { -1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f },
	{ 1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f },
	{ 1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f },
	{ 1.0f, -1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f },
	{ -1.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f },
	{ -1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { -1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f },
	{ 1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }
};

Material_Parameters material_cube;

float cube_alpha;
int flag_blend_mode = 0;

void prepare_cube(void) {
	// Initialize vertex buffer object.
	glGenBuffers(1, &cube_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, cube_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), &cube_vertices[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &cube_VAO);
	glBindVertexArray(cube_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, cube_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_cube.ambient_color[0] = 0.1745f;
	material_cube.ambient_color[1] = 0.01175f;
	material_cube.ambient_color[2] = 0.01175f;
	material_cube.ambient_color[3] = 1.0f;

	material_cube.diffuse_color[0] = 0.61424f;
	material_cube.diffuse_color[1] = 0.04136f;
	material_cube.diffuse_color[2] = 0.04136f;
	material_cube.diffuse_color[3] = 1.0f;

	material_cube.specular_color[0] = 0.727811f;
	material_cube.specular_color[1] = 0.626959f;
	material_cube.specular_color[2] = 0.626959f;
	material_cube.specular_color[3] = 1.0f;

	material_cube.specular_exponent = 20.0f;

	material_cube.emissive_color[0] = 0.3f;
	material_cube.emissive_color[1] = 0.05f;
	material_cube.emissive_color[2] = 0.05f;
	material_cube.emissive_color[3] = 1.0f;

	cube_alpha = 0.5f;
}

GLuint rectangle_VBO, rectangle_VAO;
GLfloat rectangle_vertices[12][3] = {  // vertices enumerated counterclockwise
	 { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },
	 { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },
	 { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
};
Material_Parameters material_screen;

void prepare_rectangle(void) { // Draw coordinate axes.

	glGenBuffers(1, &rectangle_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), &rectangle_vertices[0][0], GL_STATIC_DRAW);
	
	// Initialize vertex array object.
	glGenVertexArrays(1, &rectangle_VAO);
	glBindVertexArray(rectangle_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	glVertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	

	material_screen.ambient_color[0] = 0.24725f;
	material_screen.ambient_color[1] = 0.1995f;
	material_screen.ambient_color[2] = 0.0745f;
	material_screen.ambient_color[3] = 1.0f;

	material_screen.diffuse_color[0] = 0.75164f;
	material_screen.diffuse_color[1] = 0.60648f;
	material_screen.diffuse_color[2] = 0.22648;
	material_screen.diffuse_color[3] = 1.0f;

	material_screen.specular_color[0] = 0.628281f;
	material_screen.specular_color[1] = 0.555802f;
	material_screen.specular_color[2] = 0.366065f;
	material_screen.specular_color[3] = 1.0f;

	material_screen.specular_exponent = 51.2f;

	material_screen.emissive_color[0] = 0.6f;
	material_screen.emissive_color[1] = 0.6f;
	material_screen.emissive_color[2] = 0.1f;
	material_screen.emissive_color[3] = 1.0f;
}
void set_material_cube(void) {
	glUniform4fv(loc_material.ambient_color, 1, material_cube.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_cube.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_cube.specular_color);
	glUniform1f(loc_material.specular_exponent, material_cube.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_cube.emissive_color);
}

int flag_draw_screen, flag_screen_effect;
float screen_frequency, screen_width;

void set_material_screen(void) {
	glUniform4fv(loc_material.ambient_color, 1, material_screen.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_screen.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_screen.specular_color);
	glUniform1f(loc_material.specular_exponent, material_screen.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_screen.emissive_color);
}
void draw_screen(void) {
	glFrontFace(GL_CCW);

	//glUseProgram(h_ShaderProgram_TXPS);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	glBindVertexArray(rectangle_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	//glUseProgram(0);
}

void initialize_screen(void) {
	flag_draw_screen = 0;
	flag_screen_effect = 0;
	screen_frequency = 1.0f;
	screen_width = 0.125f;
}
void draw_sun_temple(void) {
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform1i(loc_screen_effect, 0);
	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1f(loc_u_fragment_alpha, tiger_alpha);
	ModelViewMatrix = ViewMatrix;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	for (int materialIdx = 0; materialIdx < scene.n_materials; materialIdx++) {
		// set material
		glUniform4fv(loc_material.ambient_color, 1, scene.material_list[materialIdx].shading.ph.ka);
		glUniform4fv(loc_material.diffuse_color, 1, scene.material_list[materialIdx].shading.ph.kd);
		glUniform4fv(loc_material.specular_color, 1, scene.material_list[materialIdx].shading.ph.ks);
		glUniform1f(loc_material.specular_exponent, scene.material_list[materialIdx].shading.ph.spec_exp);
		glUniform4f(loc_material.emissive_color, 0.0f, 0.0f, 0.0f, 1.0f);

		int texId = scene.material_list[materialIdx].diffuseTexId;
		glUniform1i(loc_texture, texId);
		glUniform1i(loc_flag_texture_mapping, flag_texture_mapping[texId]);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0 + texId);
		glBindTexture(GL_TEXTURE_2D, sun_temple_texture_names[texId]);

		glBindVertexArray(sun_temple_VAO[materialIdx]);
		glDrawArrays(GL_TRIANGLES, 0, 3 * sun_temple_n_triangles[materialIdx]);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	}
	glUseProgram(0);
}

void draw_cube(void) {
	glFrontFace(GL_CW);
	glUniform1i(loc_screen_effect, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(cube_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 6);
	glBindVertexArray(0);
}

void draw_mytiger_20181632(void) {
	glFrontFace(GL_CCW);

	glUseProgram(h_ShaderProgram_simple);
	glUniform3f(loc_primitive_color, 0.0f, 1.0f, 1.0f); // Tiger wireframe color = cyan
	
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);
	
}



void draw_myben_20181632(void) {
	glFrontFace(GL_CW);

	int bf = 0;
	int bf1 = 0;
	int ben_timer = timestamp % 720;
	int ben_timer2 = timestamp % 1440;
	if (ben_timer > 360) {
		ben_timer = 720 - ben_timer;
		bf = 1;

	}
	//if (ben_timer2 > 720) {
		//ben_timer2 = 1440 - ben_timer2;
		//bf1 = 1;

//	}
	glUseProgram(h_ShaderProgram_simple);
	set_material_tiger();
	//glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-100.0f, -5000.0f, 50.0f));


	if (ben_timer2 >= 360 && ben_timer2 <= 1080) {
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(0.0f, -1080.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, rotation_angle_tiger * 5, glm::vec3(0.0f, 0.0f, 1.0f));
		//ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(100.0f, 0.0f, 0.0f));

	}
	else {
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(0.0f, -(float)ben_timer * 3, 0.0f));
	}
	if (bf)
	{
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(200.0f, 200.0f, 200.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));
	glUniform3f(loc_primitive_color, 204.0f/255.0f, 204.0f / 255.0f, 1.0f);
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindVertexArray(ben_VAO);
	glDrawArrays(GL_TRIANGLES, ben_vertex_offset[cur_frame_ben], 3 * ben_n_triangles[cur_frame_ben]);
	glBindVertexArray(0);
}

void draw_myironman_20181632(void) {
	glFrontFace(GL_CW);
	if (flag_shading) {
		glUseProgram(h_ShaderProgram_GRD);
		set_material_ironman();


		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-50.0f, -2500.0f, 100.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(50.0f, 50.0f, 50.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
		ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_GRD, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glUniformMatrix4fv(loc_ModelViewMatrix_GRD, 1, GL_FALSE, &ModelViewMatrix[0][0]);
		glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_GRD, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(ironman_VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
		glBindVertexArray(0);
	}
	else
	{
		glUseProgram(h_ShaderProgram_TXPS);
		set_material_ironman1();


		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-50.0f, -2500.0f, 100.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(50.0f, 50.0f, 50.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
		ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
		glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(ironman_VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
		glBindVertexArray(0);
	}
	
}

void draw_mydragon_20181632(void) {
	glFrontFace(GL_CW);

	glUseProgram(h_ShaderProgram_simple);
	set_material_tiger();

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-50.0f, -6000.0f, 500.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.5f, 1.0f));
	//ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(5.0f, 5.0f, 5.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));


	glUniform3f(loc_primitive_color, 204.0f / 255.0f, 102.0f / 255.0f, 0.0f);
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindVertexArray(dragon_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * dragon_n_triangles);
	glBindVertexArray(0);
}

void draw_mycow_20181632(void) {
	glFrontFace(GL_CW);

	if (flag_shading)
	{
		glUseProgram(h_ShaderProgram_TXPS);
		set_material_tiger();
		glUniform1f(loc_u_fragment_alpha, tiger_alpha);
		glUniform1i(loc_screen_effect, 0);
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(100.0f, -2500.0f, 100.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(200.0f, 200.0f, 200.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
		ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
		glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(cow_VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3 * cow_n_triangles);
		glBindVertexArray(0);
	}
	else
	{
		glUseProgram(h_ShaderProgram_GRD);
		//printf("gorad");
		set_material_tiger1();
		glUniform1f(loc_u_fragment_alpha, tiger_alpha);
		glUniform1i(loc_screen_effect, 0);
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(100.0f, -2500.0f, 100.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(200.0f, 200.0f, 200.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
		ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_GRD, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glUniformMatrix4fv(loc_ModelViewMatrix_GRD, 1, GL_FALSE, &ModelViewMatrix[0][0]);
		glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_GRD, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
		
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(cow_VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3 * cow_n_triangles);
		glBindVertexArray(0);
	}
	
}

void draw_myoptimus_20181632(void) {
	glFrontFace(GL_CW);

	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_ROBOT);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-500.0f, -2000.0f, 240.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	//glUniform3f(loc_primitive_color, 102.0f/255.0f, 0.0f, 204.0f/255.0f);
	//glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(optimus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * optimus_n_triangles);
	glBindVertexArray(0);
}

void draw_myspider_20181632(void) {
	
	glFrontFace(GL_CW);
	int sf = 0;
	int sf1 = 0;
	int spider_timer = timestamp % 350;
	int spider_timer2 = timestamp % 700;
	if (spider_timer > 175) {
		spider_timer = 350 - spider_timer;
		sf = 1;

	}
	if (spider_timer2 > 350) {
		spider_timer2 = 700 - spider_timer2;
		sf1 = 1;

	}

	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_SPIDER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-100.0f, -6750.0f, 400.0f));
	if (spider_timer2 >= 175) {
		sf = 0;
		if (sf1)
			sf = 1;
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(((float)spider_timer2 - 175), 0.0f, -200.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90 * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	else {
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(0.0f, 0.0f, -(float)spider_timer));
	}
	if (sf) {
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -180 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(30.0f, 30.0f, 30.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	//glUniform3f(loc_primitive_color, 0.0f, 32.0f/255.0f, 32.0f/255.0f);

	//glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(spider_VAO);
	glDrawArrays(GL_TRIANGLES, spider_vertex_offset[cur_frame_spider], 3 * spider_n_triangles[cur_frame_spider]);
	glBindVertexArray(0);
}

void draw_mybike_20181632(void) {
	glFrontFace(GL_CW);

	glUseProgram(h_ShaderProgram_simple);
	set_material_tiger();
	//glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(200.0f, -6000.0f, 100.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(50.0f, 50.0f, 50.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

	glUniform3f(loc_primitive_color, 1.0f, 1.0f, 0.0f);
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindVertexArray(bike_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * bike_n_triangles);
	glBindVertexArray(0);
}
/*****************************  END: geometry setup *****************************/

/********************  START: callback function definitions *********************/
static int prevx, prevy;
float rotation_angle_cube = 0.0f;

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (flag_blend_mode) {
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	}
	draw_grid();
	draw_axes();
	draw_sun_temple();
	if (t||g) {
		glm::mat4 Matrix_camera_sight_inverse;
		if (t && up_down) {
			int up_down_timer = tiger_timestamp % 90;
			if (up_down_timer > 45.0f) {
				up_down_timer = 45 - up_down_timer;
			}
			if (up_down_timer > 22.5) {
				up_down_timer = 45 - up_down_timer;
			}
			if (up_down_timer < -22.5) {
				up_down_timer = -45 - up_down_timer;
			}
			
				ModelMatrix_tiger_sight = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -88.0f, 62.0f));
				ModelMatrix_tiger_sight = glm::rotate(ModelMatrix_tiger_sight, (float)up_down_timer*0.3f*TO_RADIAN,(glm::vec3(1.0f, 0.0f, 0.0f)));
				ModelMatrix_tiger_sight = glm::rotate(ModelMatrix_tiger_sight, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
				ModelMatrix_tiger_sight = glm::rotate(ModelMatrix_tiger_sight, 180 * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		}	
		Matrix_camera_sight_inverse = ModelMatrix_tiger * ModelMatrix_tiger_sight;
		ViewMatrix = glm::affineInverse(Matrix_camera_sight_inverse);

		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	}
	int fb=0;
	int tg = 0;
	int Tiger_timer = tiger_timestamp % 200;
	int Tiger_timer2 = tiger_timestamp % 400;
	if (Tiger_timer2 >= 200) {
		tg = 1;
		Tiger_timer2 = 400 - Tiger_timer2;
		
	}
		if (Tiger_timer > 100) {
			Tiger_timer = 100 - Tiger_timer;
			fb = 1;
		
			
		}
		if (Tiger_timer >= 50) {
			fb = 1;
			Tiger_timer = 100 - Tiger_timer;
		
			
		}
		if (Tiger_timer <= -50) {
			fb = 0;
		
			Tiger_timer = -100 - Tiger_timer;
		}
		
	//glUseProgram(h_ShaderProgram_simple);
	ModelMatrix_tiger = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2000.0f, 0.0f));
	
	if (Tiger_timer2 >= 50&&Tiger_timer2<=90) {
		fb = 0;
		if (tg)
			fb = 1;
		ModelMatrix_tiger = glm::translate(ModelMatrix_tiger, glm::vec3(500, -((float)Tiger_timer2-50)*10, -((float)Tiger_timer2 - 50)*5));
		ModelMatrix_tiger = glm::rotate(ModelMatrix_tiger, -90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else if (Tiger_timer2 > 90)
	{
		
		fb = 0;
		if (tg)
			fb = 1;
		ModelMatrix_tiger = glm::translate(ModelMatrix_tiger, glm::vec3(500, -((float)Tiger_timer2 - 50) * 10, -200.0f));
		ModelMatrix_tiger = glm::rotate(ModelMatrix_tiger, 90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix_tiger = glm::rotate(ModelMatrix_tiger, 180 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else{
		if (tg && Tiger_timer2 < 50) {
			Tiger_timer *= -1;
			fb = 1;
			
		}
		ModelMatrix_tiger = glm::translate(ModelMatrix_tiger, glm::vec3((float)Tiger_timer * 10, 0.0f, 0.0f));
	}
	
	ModelMatrix_tiger = glm::translate(ModelMatrix_tiger, glm::vec3(0.0f, 0.0f, 250.0f));
	ModelMatrix_tiger = glm::rotate(ModelMatrix_tiger, 90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	if (fb) {
		ModelMatrix_tiger = glm::rotate(ModelMatrix_tiger, 180 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	ModelMatrix_tiger = glm::scale(ModelMatrix_tiger, glm::vec3(3.0f, 3.0f, 3.0f));
	//ModelViewProjectionMatrix = ProjectionMatrix * ModelMatrix_tiger;
	/*---------------------------------------------------------------------------------------------------*/

	int fb1 = 0;
	int tg1 = 0;
	//int Tiger_timer = timestamp % 200;
	//int Tiger_timer2 = timestamp % 400;
	if (Tiger_timer2 >= 200) {
		tg1 = 1;
		Tiger_timer2 = 400 - Tiger_timer2;

	}
	if (Tiger_timer > 100) {
		Tiger_timer = 100 - Tiger_timer;
		fb1 = 1;


	}
	if (Tiger_timer >= 50) {
		fb1 = 1;
		Tiger_timer = 100 - Tiger_timer;


	}
	if (Tiger_timer <= -50) {
		fb1 = 0;

		Tiger_timer = -100 - Tiger_timer;
	}

	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0.0f, -2000.0f, 0.0f));

	if (Tiger_timer2 >= 50 && Tiger_timer2 <= 90) {
		fb1 = 0;
		if (tg1)
			fb1 = 1;
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(500, -((float)Tiger_timer2 - 50) * 10, -((float)Tiger_timer2 - 50) * 5));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else if (Tiger_timer2 > 90)
	{

		fb1 = 0;
		if (tg1)
			fb1 = 1;
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(500, -((float)Tiger_timer2 - 50) * 10, -200.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else {
		if (tg1 && Tiger_timer2 < 50) {
			Tiger_timer *= -1;
			fb1 = 1;

		}
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3((float)Tiger_timer * 10, 0.0f, 0.0f));
	}

	ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(0.0f, 0.0f, 250.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	if (fb) {
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(3.0f, 3.0f, 3.0f));
	Tiger_light = ModelViewMatrix;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	
	

	draw_mytiger_20181632();

	glUseProgram(h_ShaderProgram_TXPS);
	if (flag_draw_screen) {
		set_material_screen();
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-700.0f, -2500.0f, 350.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(300.0f, 300.0f, 300.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
		ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

		glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
		glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

		glUniform1i(loc_screen_effect, flag_screen_effect);
		draw_screen();
		glUniform1i(loc_screen_effect, 0);
	}
	glUseProgram(0);
	
	glUseProgram(h_ShaderProgram_TXPS);
	if (flag_blend_mode) {
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glUniform1i(loc_u_flag_blending, 1); // Back_to_Front
	}
	else
		glUniform1i(loc_u_flag_blending, 0);

	glUseProgram(0);
	

	
	draw_myben_20181632();

	
	draw_myironman_20181632();

	
	draw_mycow_20181632();

	
	draw_myoptimus_20181632();

	
	draw_mybike_20181632();

	

	draw_mydragon_20181632();

	light[0].position[0] = 50.0f; light[0].position[1] = -2400.0f; // spot light position in WC
	light[0].position[2] = 1000.0f; light[0].position[3] = 1.0f;
	light[0].ambient_color[0] = 0.0215f; light[0].ambient_color[1] = 0.1745f;
	light[0].ambient_color[2] = 0.0215f; light[0].ambient_color[3] = 1.0f;

	light[0].diffuse_color[0] = 0.07568f; light[0].diffuse_color[1] = 0.61424f;
	light[0].diffuse_color[2] = 0.07568f; light[0].diffuse_color[3] = 1.0f;

	light[0].specular_color[0] =0.633f; light[0].specular_color[1] = 0.727811f;
	light[0].specular_color[2] = 0.633f; light[0].specular_color[3] = 1.0f;

	light[0].spot_direction[0] = 0.0f; light[0].spot_direction[1] = 0.0f; // spot light direction in WC
	light[0].spot_direction[2] = -1.0f;
	light[0].spot_cutoff_angle = 15.0f;
	light[0].spot_exponent = 4.0f;

	glUseProgram(h_ShaderProgram_TXPS);
	//glUniform1i(loc_blind_effect, 0);
	glUniform1i(loc_light[14].light_on, light[0].light_on);
	glm::vec4 position_EC = ViewMatrix * glm::vec4(light[0].position[0], light[0].position[1],
		light[0].position[2], light[0].position[3]);
	glUniform4fv(loc_light[14].position, 1, &position_EC[0]);
	glUniform4fv(loc_light[14].ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_light[14].diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_light[14].specular_color, 1, light[0].specular_color);
	glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[0].spot_direction[0], light[0].spot_direction[1], light[0].spot_direction[2]);
	glUniform3fv(loc_light[14].spot_direction, 1, &direction_EC[0]);
	glUniform1f(loc_light[14].spot_cutoff_angle, light[0].spot_cutoff_angle);
	glUniform1f(loc_light[14].spot_exponent, light[0].spot_exponent);
	
	glUseProgram(0);

	glUseProgram(h_ShaderProgram_GRD);
	glUniform1i(loc_light1[14].light_on, light[0].light_on);
	
	glm::vec4 position_EC_GR = ViewMatrix * glm::vec4(light[0].position[0], light[0].position[1],
		light[0].position[2], light[0].position[3]);
	glUniform4fv(loc_light1[14].position, 1, &position_EC[0]);
	glUniform4fv(loc_light1[14].ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_light1[14].diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_light1[14].specular_color, 1, light[0].specular_color);
	glm::vec3 direction_EC_GR = glm::mat3(ViewMatrix) * glm::vec3(light[0].spot_direction[0], light[0].spot_direction[1], light[0].spot_direction[2]);
	glUniform3fv(loc_light1[14].spot_direction, 1, &direction_EC[0]);
	glUniform1f(loc_light1[14].spot_cutoff_angle, light[0].spot_cutoff_angle);
	glUniform1f(loc_light1[14].spot_exponent, light[0].spot_exponent);
	glUseProgram(0);

	/*-------------------------------------------------------------------------------------------------------------------------------------------*/

	light[2].position[0] = 0.0f; light[2].position[1] = 0.0f; // spot light position in WC
	light[2].position[2] = 20.0f; light[2].position[3] = 1.0f;
	light[2].ambient_color[0] = 0.1745f; light[2].ambient_color[1] = 0.01175f;
	light[2].ambient_color[2] = 0.01175f; light[2].ambient_color[3] = 1.0f;

	light[2].diffuse_color[0] = 0.61424f; light[2].diffuse_color[1] = 0.04136f;
	light[2].diffuse_color[2] = 0.04136f; light[2].diffuse_color[3] = 1.0f;

	light[2].specular_color[0] = 0.728211f; light[2].specular_color[1] = 0.626959f;
	light[2].specular_color[2] = 0.626959f; light[2].specular_color[3] = 1.0f;

	light[2].spot_direction[0] = 0.0f; light[2].spot_direction[1] = -1.0f; // spot light direction in WC
	light[2].spot_direction[2] = 0.0f;
	light[2].spot_cutoff_angle = 15.0f;
	light[2].spot_exponent = 3.0f;

	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_light[15].light_on, light[2].light_on);
	glm::vec4 position_EC_TG = Tiger_light * glm::vec4(light[2].position[0], light[2].position[1],
		light[2].position[2], light[2].position[3]);
	glUniform4fv(loc_light[15].position, 1, &position_EC_TG[0]);
	glUniform4fv(loc_light[15].ambient_color, 1, light[2].ambient_color);
	glUniform4fv(loc_light[15].diffuse_color, 1, light[2].diffuse_color);
	glUniform4fv(loc_light[15].specular_color, 1, light[2].specular_color);
	glm::vec3 direction_EC_TG = glm::mat3(Tiger_light) * glm::vec3(light[2].spot_direction[0], light[2].spot_direction[1], light[2].spot_direction[2]);
	glUniform3fv(loc_light[15].spot_direction, 1, &direction_EC_TG[0]);
	glUniform1f(loc_light[15].spot_cutoff_angle, light[2].spot_cutoff_angle);
	glUniform1f(loc_light[15].spot_exponent, light[2].spot_exponent);
	
	glUseProgram(0);

	glUseProgram(h_ShaderProgram_GRD);
	glUniform1i(loc_light1[15].light_on, light[2].light_on);
	glUniform4fv(loc_light1[15].position, 1, &position_EC_TG[0]);
	glUniform4fv(loc_light1[15].ambient_color, 1, light[2].ambient_color);
	glUniform4fv(loc_light1[15].diffuse_color, 1, light[2].diffuse_color);
	glUniform4fv(loc_light1[15].specular_color, 1, light[2].specular_color);
	
	glUniform3fv(loc_light1[15].spot_direction, 1, &direction_EC_TG[0]);
	glUniform1f(loc_light1[15].spot_cutoff_angle, light[2].spot_cutoff_angle);
	glUniform1f(loc_light1[15].spot_exponent, light[2].spot_exponent);
	glUseProgram(0);
	
	glEnable(GL_CULL_FACE);
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_cube();
	glUniform1f(loc_u_fragment_alpha, cube_alpha);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(500.0f, -2800.0f, 550.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, rotation_angle_cube, glm::vec3(1.0f, 1.0f, 1.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glCullFace(GL_BACK);
	draw_cube();

	glCullFace(GL_FRONT);
	draw_cube();
	glDisable(GL_CULL_FACE);
	draw_myspider_20181632();
	glutSwapBuffers();

	if (flag_blend_mode) {
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
}
void timer_scene(int timestamp_scene) {
	if (animation_flag)
	{
		tiger_timestamp = (tiger_timestamp + 1) % UINT_MAX;
		cur_frame_tiger = timestamp_scene % N_TIGER_FRAMES;
		
	}
	timestamp = (timestamp + 1) % UINT_MAX;
	
	cur_frame_spider = timestamp % N_SPIDER_FRAMES;
	cur_frame_ben = timestamp_scene % N_BEN_FRAMES;
	rotation_angle_tiger = (timestamp_scene % 720) * TO_RADIAN;
	rotation_angle_cube = (timestamp_scene % 360) * TO_RADIAN;
	glutTimerFunc(20, timer_scene, (timestamp_scene + 1) % INT_MAX);
	glutPostRedisplay();
	
}

void special(int key, int x, int y) {
	crtlpressed = glutGetModifiers();
	glm::vec3 nvex = glm::vec3(-current_camera.naxis[0], -current_camera.naxis[1], -current_camera.naxis[2]);
	glm::vec3 pos = glm::vec3(current_camera.pos[0], current_camera.pos[1], current_camera.pos[2]);
	glm::vec3 vvex = glm::vec3(current_camera.vaxis[0], current_camera.vaxis[1], current_camera.vaxis[2]);
	glm::vec3 uvex = glm::vec3(current_camera.uaxis[0], current_camera.uaxis[1], current_camera.uaxis[2]);
	if (m_mode &&!t && !g) {
		
		switch (key) {

		case GLUT_KEY_UP:
			if(crtlpressed==GLUT_ACTIVE_CTRL)
				pos += 20.0f * vvex;
			else
				pos += 20.0f * nvex;
			current_camera.pos[0] = pos[0];
			current_camera.pos[1] = pos[1];
			current_camera.pos[2] = pos[2];
			set_ViewMatrix_from_camera_frame();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			if (crtlpressed == GLUT_ACTIVE_CTRL)
				pos -= 20.0f * vvex;

			else
				pos -= 20.0f * nvex;
			current_camera.pos[0] = pos[0];
			current_camera.pos[1] = pos[1];
			current_camera.pos[2] = pos[2];
			set_ViewMatrix_from_camera_frame();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		case GLUT_KEY_LEFT:
			
			pos -= 20.0f * uvex;
			current_camera.pos[0] = pos[0];
			current_camera.pos[1] = pos[1];
			current_camera.pos[2] = pos[2];
			set_ViewMatrix_from_camera_frame();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		case GLUT_KEY_RIGHT:
			
			pos += 20.0f * uvex;
			current_camera.pos[0] = pos[0];
			current_camera.pos[1] = pos[1];
			current_camera.pos[2] = pos[2];
			set_ViewMatrix_from_camera_frame();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		}
	}
}
void keyboard(unsigned char key, int x, int y) {

	static int flag_blind_effect = 0;
	switch (key) {
	case 'p':
		animation_flag = 1 - animation_flag;
		/*if (animation_flag) {
			glutTimerFunc(100, timer_scene, 0);
		}*/
		break;
	case '1':
		if (!t&&!g) {
			set_current_camera(CAMERA_1);
			glutPostRedisplay();
		}
		break;
	case '2':
		if (!t&&!g) {
			set_current_camera(CAMERA_2);
			glutPostRedisplay();
		}
		break;
	case '3':
		if (!t&&!g) {
			set_current_camera(CAMERA_3);
			glutPostRedisplay();
		}
		break;
	case '0':
		if (!t&&!g) {
			set_current_camera(CAMERA_4);
			glutPostRedisplay();
		}
		break;
	case 'm':
		if (!t&&!g) {
			m_mode = 1 - m_mode;
			if (m_mode)
			{
				fprintf(stdout, "----Camera Moving mode----\n");
				fprintf(stdout, "----Crtl + UP or DOWN to move up and down----\n");
				fprintf(stdout, "----UP or DOWN to move front and back----\n");
				fprintf(stdout, "----RIGHT or LEFT to move right and left----\n");
			}
		}
		break;
	case 'x':
		if(m_mode && !t&&!g)
			flag_translation_axis = X_AXIS;
		break;
	case 'y':
		if (m_mode && !t&&!g)
			flag_translation_axis = Y_AXIS;
		break;
	case 'z':
		if (m_mode && !t&&!g)
			flag_translation_axis = Z_AXIS;
		break;
	case 'n':
		if (m_mode && !t&&!g)
			flag_translation_axis = NORMAL;
		break;
	case 't':
		if (!g) {
			if (t) {
				set_current_camera(t_curcam);
			}
			t = 1 - t;
			//up_down = 1 - up_down;
			if (t) {
				tiger_sight_camera();
				set_tiger_camera(CAMERA_T);
				glutPostRedisplay();
			}
			
		}
		break;
	case 'g':
		if (!t) {
			if (g) {
				set_current_camera(t_curcam);
			}
			g = 1 - g;
			if (g) {
				tiger_back_sight_camera();
				set_tiger_camera(CAMERA_G);
				glutPostRedisplay();
			}
		}
		break;
	case 'r':
		if (t) {
			up_down = 1 - up_down;
			tiger_sight_camera();
			set_tiger_camera(CAMERA_T);
			glutPostRedisplay();
		}
		break;
	case 'e':
		glUseProgram(h_ShaderProgram_TXPS);
		light[1].light_on = 1 - light[1].light_on;
		glUniform1i(loc_light[13].light_on, light[1].light_on);
		glUseProgram(0);

		glUseProgram(h_ShaderProgram_GRD);
		glUniform1i(loc_light1[13].light_on, light[1].light_on);
		glUseProgram(0);
		break;
	case 'w':
		glUseProgram(h_ShaderProgram_TXPS);
		light[0].light_on = 1 - light[0].light_on;
		glUniform1i(loc_light[14].light_on, light[0].light_on);
		glUseProgram(0);

		glUseProgram(h_ShaderProgram_GRD);
		glUniform1i(loc_light1[14].light_on, light[0].light_on);
		glUseProgram(0);
		break;
	case 'k':
		glUseProgram(h_ShaderProgram_TXPS);
		light[2].light_on = 1 - light[2].light_on;
		glUniform1i(loc_light[15].light_on, light[2].light_on);
		glUseProgram(0);

		glUseProgram(h_ShaderProgram_GRD);
		glUniform1i(loc_light1[15].light_on, light[2].light_on);
		glUseProgram(0);
		break;
	case 'c':
		flag_blend_mode = 1 - flag_blend_mode;
		if (flag_blend_mode) {
			printf("Blending mode on\n");
		}
		else
			printf("Blending mode off\n");
		break;
	case 'a':
		if (flag_blend_mode) {
			cube_alpha += 0.05f;
			if (cube_alpha > 1.0f)
				cube_alpha = 1.0f;
			glutPostRedisplay();
		}
		break;
	case 'A':
		if (flag_blend_mode) {
			cube_alpha -= 0.05f;
			if (cube_alpha < 0.05f)
				cube_alpha = 0.05f;
			glutPostRedisplay();
		}
		break;
	case 'b':
		flag_blind_effect = 1 - flag_blind_effect;
		glUseProgram(h_ShaderProgram_TXPS);
		glUniform1i(loc_blind_effect, flag_blind_effect);
		glUseProgram(0);
		glUseProgram(h_ShaderProgram_GRD);
		glUniform1i(loc_blind_effect1, flag_blind_effect);
		glUseProgram(0);
		glutPostRedisplay();
		break;
	case 'h':
		if (flag_draw_screen) {
			flag_screen_effect = 1 - flag_screen_effect;
			glutPostRedisplay();
		}
		break;
	case 'j':
		flag_shading = 1 - flag_shading;
			
		glutPostRedisplay();
		break;
	case 's':
		flag_draw_screen = 1 - flag_draw_screen;
		glutPostRedisplay();
		break;
#define SCEEN_MAX_FREQUENCY 50.0f
	case 'd':
		if (flag_draw_screen) {
			screen_frequency += 1.0f;
			if (screen_frequency > SCEEN_MAX_FREQUENCY)
				screen_frequency = SCEEN_MAX_FREQUENCY;
			glUseProgram(h_ShaderProgram_TXPS);
			glUniform1f(loc_screen_frequency, screen_frequency);
			glUseProgram(0);
			glutPostRedisplay();
		}
		break;
#define SCEEN_MIN_FREQUENCY 1.0f
	case 'f':
		if (flag_draw_screen) {
			screen_frequency -= 1.0f;
			if (screen_frequency < SCEEN_MIN_FREQUENCY)
				screen_frequency = SCEEN_MIN_FREQUENCY;
			glUseProgram(h_ShaderProgram_TXPS);
			glUniform1f(loc_screen_frequency, screen_frequency);
			glUseProgram(0);
			glutPostRedisplay();
		}
		break;
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}


#define CAM_ZOOM_STEP			0.05f
#define CAM_MAX_ZOOM_IN_FACTOR  0.25f
#define CAM_MAX_ZOOM_OUT_FACTOR	2.5f

void mousewheel(int button, int dir, int x, int y) {
	shiftpressed = glutGetModifiers();
	Camera* camera = &current_camera;
	if (shiftpressed == GLUT_ACTIVE_SHIFT) {
		if (dir > 0) {
			camera->zoom_factor -= CAM_ZOOM_STEP;
			if (camera->zoom_factor < CAM_MAX_ZOOM_IN_FACTOR)
				camera->zoom_factor = CAM_MAX_ZOOM_IN_FACTOR;
			ProjectionMatrix = glm::perspective(camera->zoom_factor * camera->fovy, camera->aspect_ratio, camera->near_c, camera->far_c);
			
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
		}
		else {
			camera->zoom_factor += CAM_ZOOM_STEP;
			if (camera->zoom_factor > CAM_MAX_ZOOM_OUT_FACTOR)
				camera->zoom_factor = CAM_MAX_ZOOM_OUT_FACTOR;
			ProjectionMatrix = glm::perspective(camera->zoom_factor * camera->fovy, camera->aspect_ratio, camera->near_c, camera->far_c);
			
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
		}
		
	}
}
void motion(int x, int y) {
	if (!current_camera.move) return;
	if (m_mode && !t&&!g) {
		switch (flag_translation_axis) {
		case X_AXIS:
			renew_cam_orientation_rotation_around_u_axis(prevy - y);
			break;
		case Y_AXIS:
			renew_cam_orientation_rotation_around_n_axis(prevx - x);
			break;
		case Z_AXIS:
			renew_cam_orientation_rotation_around_v_axis(prevx - x);
			break;
		case NORMAL:
			renew_cam_orientation_rotation_around_u_axis(prevy - y);
			renew_cam_orientation_rotation_around_v_axis(prevx - x);
			break;
		}
	}
	

	prevx = x; prevy = y;

	set_ViewMatrix_from_camera_frame();
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON)&& !t && !g) {
		if (state == GLUT_DOWN) {
			current_camera.move = 1;
			prevx = x; prevy = y;
		}
		else if (state == GLUT_UP) current_camera.move = 0;
	}
}
void reshape(int width, int height) {
	float aspect_ratio;

	glViewport(0, 0, width, height);
	ProjectionMatrix = glm::perspective(current_camera.fovy, current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	glutPostRedisplay();
}

void cleanup(void) {
	glDeleteVertexArrays(1, &axes_VAO);
	glDeleteBuffers(1, &axes_VBO);

	glDeleteVertexArrays(1, &grid_VAO);
	glDeleteBuffers(1, &grid_VBO);

	glDeleteVertexArrays(scene.n_materials, sun_temple_VAO);
	glDeleteBuffers(scene.n_materials, sun_temple_VBO);

	glDeleteVertexArrays(1, &tiger_VAO);
	glDeleteBuffers(1, &tiger_VBO);

	glDeleteTextures(scene.n_textures, sun_temple_texture_names);
	glDeleteTextures(N_TEXTURES_USED, texture_names);

	free(sun_temple_n_triangles);
	free(sun_temple_vertex_offset);

	free(sun_temple_VAO);
	free(sun_temple_VBO);

	free(sun_temple_texture_names);
	free(flag_texture_mapping);
}
/*********************  END: callback function definitions **********************/



void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutSpecialFunc(special);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup);
	glutMouseWheelFunc(mousewheel);
}

void initialize_OpenGL(void) {
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	ViewMatrix = glm::mat4(1.0f);
	ProjectionMatrix = glm::mat4(1.0f);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	initialize_lights();
	initialize_flags();
	glGenTextures(N_TEXTURES_USED, texture_names);

	flag_translation_axis = NORMAL;
	
}

void prepare_scene(void) {
	prepare_axes();
	prepare_grid();
	prepare_sun_temple();
	prepare_tiger();
	prepare_spider();
	prepare_ben();
	prepare_ironman();
	prepare_cow();
	prepare_optimus();
	prepare_bike();
	prepare_dragon();
	prepare_cube();
	prepare_rectangle();
	initialize_screen();
	set_light();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
	initialize_camera();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "********************************************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "********************************************************************************\n\n");
}

void print_message(const char* m) {
	fprintf(stdout, "%s\n\n", m);
}

void greetings(char* program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "********************************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n********************************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 15
void drawScene(int argc, char* argv[]) {
	char program_name[64] = "Sogang CSE4170 Sun Temple Scene by 20181632 박성현";
	char messages[N_MESSAGE_LINES][256] = { 
		"    - Keys used:",
		"		'0' : set the camera for top view",
		"		'1' : set the camera for original view",
		"		'2' : set the camera for bronze statue view",
		"		'3' : set the camera for bronze statue view",	
		"       'x,y,z' : rotate around given axis",
		"       'n' : return to normal rotation",
		"       't' : tiger sight camera, 'r' : moving up_down",
		"       'g' : tiger back sight camera",
		"		'ESC' : program close",
	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(900, 600);
	glutInitWindowPosition(20, 20);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}
