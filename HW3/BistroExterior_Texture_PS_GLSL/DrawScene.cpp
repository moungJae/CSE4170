//
//  DrawScene.cpp
//
//  Written for CSE4170
//  Department of Computer Science and Engineering
//  Copyright © 2022 Sogang University. All rights reserved.
//
//	20192138 조명재
//	HW 3: OpenGL API를 사용한 3D 뷰잉 연습
//	담당교수: 임 인 성

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

extern SCENE scene;

// for simple shaders
GLuint h_ShaderProgram_simple, h_ShaderProgram_TXPS; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

// for Phong Shading (Textured) shaders
#define NUMBER_OF_LIGHT_SUPPORTED 4 
GLint loc_global_ambient_color;
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED];
loc_Material_Parameters loc_material;
GLint loc_ModelViewProjectionMatrix_TXPS, loc_ModelViewMatrix_TXPS, loc_ModelViewMatrixInvTrans_TXPS;
GLint loc_texture;
GLint loc_flag_texture_mapping;
GLint loc_flag_fog;

Light_Parameters light[NUMBER_OF_LIGHT_SUPPORTED];

// include glm/*.hpp only if necessary
// #include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
#include <glm/gtc/matrix_inverse.hpp> // inverseTranspose, etc.
// ViewProjectionMatrix = ProjectionMatrix * ViewMatrix
glm::mat4 ViewProjectionMatrix, ViewMatrix, ProjectionMatrix;
// ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix
glm::mat4 ModelViewProjectionMatrix; // This one is sent to vertex shader when ready.
glm::mat4 ModelViewMatrix;
glm::mat3 ModelViewMatrixInvTrans;

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f

#define LOC_VERTEX 0
#define LOC_NORMAL 1
#define LOC_TEXCOORD 2

#define CAM_TSPEED 1.0f
#define CAM_RSPEED 0.1f

#define N_TEXTURES_USED 1
#define TEXTURE_ID_TIGER 0
GLuint texture_names[N_TEXTURES_USED];

unsigned int timestamp_scene = 0; // the global clock in the scene
glm::mat4 ModelMatrix_Tiger, ModelMatrix_Tiger_Eye;

/*********************************  START: camera *********************************/

typedef enum {
	CAMERA_M,	// 세상 이동 카메라
	CAMERA_0,	// 호랑이를 바라보는 카메라(1) 
	CAMERA_1,	// 호랑이를 바라보는 카메라(2)
	CAMERA_2,	// 용을 바라보는 카메라
	CAMERA_3,	// 사람을 바라보는 카메라
	CAMERA_T,	// 호랑이 관점 카메라
	CAMERA_G,	// 호랑이 관찰 카메라
	NUM_CAMERAS
} CAMERA_INDEX;

typedef struct _Camera {
	glm::vec3 prp, vrp, vup;
	float pos[3]; 
	float uaxis[3], vaxis[3], naxis[3];
	float fovy, aspect_ratio, near_c, far_c, zoom_factor;
	int move, rotation_axis;
	bool fixed;
} Camera;

Camera camera_info[NUM_CAMERAS];
Camera current_camera;
enum axes { X_AXIS, Y_AXIS, Z_AXIS, U_AXIS, V_AXIS, N_AXIS };
static int flag_translation_axis;
static int flag_rotation_axis;
int prevx, prevy;
bool tiger_camera = false;
bool tiger_follow_camera = false;
const bool CAMERA_NON_FIXED = false;
const bool CAMERA_FIXED = true;
glm::vec3 tiger_coordinate[721];

using glm::mat4;

void set_ViewMatrix_from_camera_frame() {
	if (current_camera.fixed) {
		ViewMatrix = glm::lookAt(current_camera.prp, current_camera.vrp, current_camera.vup);
		ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy,
			current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
	}
	else {
		ViewMatrix = glm::mat4(current_camera.uaxis[0], current_camera.vaxis[0], current_camera.naxis[0], 0.0f,
			current_camera.uaxis[1], current_camera.vaxis[1], current_camera.naxis[1], 0.0f,
			current_camera.uaxis[2], current_camera.vaxis[2], current_camera.naxis[2], 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
		ViewMatrix = glm::translate(ViewMatrix, glm::vec3(-current_camera.pos[0], -current_camera.pos[1], -current_camera.pos[2]));
		ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy,
			current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
	}
}

void set_current_camera(int camera_num) {
	Camera* pCamera = &camera_info[camera_num];
	glm::mat4 Matrix_CAMERA_tiger_inverse;
	glm::vec3 follow_coordinate;
	int tiger_Ptime, tiger_Vtime;

	memcpy(&current_camera, pCamera, sizeof(Camera));
	if (camera_num == CAMERA_T) {
		tiger_camera = true, tiger_follow_camera = false;
		Matrix_CAMERA_tiger_inverse = ModelMatrix_Tiger * ModelMatrix_Tiger_Eye;
		ViewMatrix = glm::affineInverse(Matrix_CAMERA_tiger_inverse);
		ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy,
			current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		return;
	}
	else if (camera_num == CAMERA_G) {
		tiger_camera = false, tiger_follow_camera = true;
		tiger_Vtime = timestamp_scene % 721;
		if (tiger_Vtime >= 180 && tiger_Vtime < 240) tiger_Ptime = 120;
		else if (tiger_Vtime >= 240 && tiger_Vtime < 300) tiger_Ptime = tiger_Vtime - 120;
		else if (tiger_Vtime >= 480 && tiger_Vtime < 540) tiger_Ptime = 420;
		else if (tiger_Vtime >= 540 && tiger_Vtime < 600) tiger_Ptime = tiger_Vtime - 120;
		else tiger_Ptime = (tiger_Vtime + 660) % 721;
		
		follow_coordinate = tiger_coordinate[tiger_Ptime] + glm::vec3(0.0f, 0.0f, 300.0f);
		ViewMatrix = glm::lookAt(follow_coordinate, tiger_coordinate[tiger_Vtime], current_camera.vup);
		ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy,
			current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		return;
	}
	else {
		tiger_camera = false, tiger_follow_camera = false;
		set_ViewMatrix_from_camera_frame();
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	}
}

void renew_cam_position(int del) {
	switch (flag_translation_axis) {
	case X_AXIS:
		current_camera.pos[0] += CAM_TSPEED * del * current_camera.uaxis[0];
		current_camera.pos[1] += CAM_TSPEED * del * current_camera.uaxis[1];
		current_camera.pos[2] += CAM_TSPEED * del * current_camera.uaxis[2];
		break;
	case Y_AXIS:
		current_camera.pos[0] += CAM_TSPEED * del * current_camera.vaxis[0];
		current_camera.pos[1] += CAM_TSPEED * del * current_camera.vaxis[1];
		current_camera.pos[2] += CAM_TSPEED * del * current_camera.vaxis[2];
		break;
	case Z_AXIS:
		current_camera.pos[0] += CAM_TSPEED * del * (-current_camera.naxis[0]);
		current_camera.pos[1] += CAM_TSPEED * del * (-current_camera.naxis[1]);
		current_camera.pos[2] += CAM_TSPEED * del * (-current_camera.naxis[2]);
		break;
	}
}

void renew_cam_orientation_rotation_around_all_axis(int angle) {
	glm::mat3 RotationMatrix;
	glm::vec3 direction;
	glm::vec3 uaxis, vaxis, naxis;

	uaxis = glm::vec3(current_camera.uaxis[0], current_camera.uaxis[1], current_camera.uaxis[2]);
	vaxis = glm::vec3(current_camera.vaxis[0], current_camera.vaxis[1], current_camera.vaxis[2]);
	naxis = glm::vec3(current_camera.naxis[0], current_camera.naxis[1], current_camera.naxis[2]);

	switch (flag_rotation_axis) {
	case U_AXIS:
		RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle, uaxis));
		direction = RotationMatrix * vaxis;
		current_camera.vaxis[0] = direction[0]; current_camera.vaxis[1] = direction[1]; current_camera.vaxis[2] = direction[2];
		direction = RotationMatrix * naxis;
		current_camera.naxis[0] = direction[0]; current_camera.naxis[1] = direction[1]; current_camera.naxis[2] = direction[2];
		break;
	case V_AXIS:
		RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle, vaxis));
		direction = RotationMatrix * uaxis;
		current_camera.uaxis[0] = direction[0]; current_camera.uaxis[1] = direction[1]; current_camera.uaxis[2] = direction[2];
		direction = RotationMatrix * naxis;
		current_camera.naxis[0] = direction[0]; current_camera.naxis[1] = direction[1]; current_camera.naxis[2] = direction[2];
		break;
	case N_AXIS:
		RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), CAM_RSPEED * TO_RADIAN * angle, naxis));
		direction = RotationMatrix * uaxis;
		current_camera.uaxis[0] = direction[0]; current_camera.uaxis[1] = direction[1]; current_camera.uaxis[2] = direction[2];
		direction = RotationMatrix * vaxis;
		current_camera.vaxis[0] = direction[0]; current_camera.vaxis[1] = direction[1]; current_camera.vaxis[2] = direction[2];
		break;
	}
}

void initialize_camera(void) {
	Camera* pCamera;
	glm::mat4 tmpMatrix;
	float move_position[3] = { -1177.64f, 4080.32f, 299.0f };

	//CAMERA_M : Move Camera
	pCamera = &camera_info[CAMERA_M];

	for (int k = 0; k < 3; k++)
	{
		pCamera->pos[k] = move_position[k];
		pCamera->uaxis[k] = scene.camera.u[k];
		pCamera->vaxis[k] = scene.camera.v[k];
		pCamera->naxis[k] = scene.camera.n[k];
	}
	pCamera->move = 0, pCamera->fixed = CAMERA_NON_FIXED;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect;
	pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f, pCamera->zoom_factor = 1.0f;

	//CAMERA_0 : Tiger View - 1
	pCamera = &camera_info[CAMERA_0];
	pCamera->prp = glm::vec3(185.0f, -920.0f, 235.0f);
	pCamera->vrp = glm::vec3(4424.0f, -2477.0f, 235.0f);
	pCamera->vup = glm::vec3(0.0f, 0.0f, 1.0f);
	tmpMatrix = glm::lookAt(pCamera->prp, pCamera->vrp, pCamera->vup);
	pCamera->vup = glm::vec3(tmpMatrix[0].y, tmpMatrix[1].y, tmpMatrix[2].y); 

	pCamera->move = 0, pCamera->fixed = CAMERA_FIXED;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect;
	pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f, pCamera->zoom_factor = 1.0f;
	
	// CAMERA_1 : Tiger View - 2
	pCamera = &camera_info[CAMERA_1];
	pCamera->prp = glm::vec3(4557.0f, -4637.0f, 1610.0f);
	pCamera->vrp = glm::vec3(4424.0f, -2477.0f, 235.0f);
	pCamera->vup = glm::vec3(0.0f, 0.0f, 1.0f);
	tmpMatrix = glm::lookAt(pCamera->prp, pCamera->vrp, pCamera->vup);
	pCamera->vup = glm::vec3(tmpMatrix[0].y, tmpMatrix[1].y, tmpMatrix[2].y);

	pCamera->move = 0, pCamera->fixed = CAMERA_FIXED;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect;
	pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f, pCamera->zoom_factor = 1.0f;

	//CAMERA_2 : Dragon View
	pCamera = &camera_info[CAMERA_2];
	pCamera->prp = glm::vec3(-427.0f, -61.0f, 145.0f);
	pCamera->vrp = glm::vec3(-2795.0f, 1748.0f, 1055.0f);
	pCamera->vup = glm::vec3(0.0f, 0.0f, 1.0f);
	tmpMatrix = glm::lookAt(pCamera->prp, pCamera->vrp, pCamera->vup);
	pCamera->vup = glm::vec3(tmpMatrix[0].y, tmpMatrix[1].y, tmpMatrix[2].y);

	pCamera->move = 0, pCamera->fixed = CAMERA_FIXED;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect;
	pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f, pCamera->zoom_factor = 1.0f;

	//CAMERA_3 : Ben View
	pCamera = &camera_info[CAMERA_3];
	pCamera->prp = glm::vec3(87.6f, 898.3f, 128.0f);
	pCamera->vrp = glm::vec3(1729.0f, 4374.0f, 404.0f);
	pCamera->vup = glm::vec3(0.0f, 0.0f, 1.0f);
	tmpMatrix = glm::lookAt(pCamera->prp, pCamera->vrp, pCamera->vup);
	pCamera->vup = glm::vec3(tmpMatrix[0].y, tmpMatrix[1].y, tmpMatrix[2].y);

	pCamera->move = 0, pCamera->fixed = CAMERA_FIXED;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect;
	pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f, pCamera->zoom_factor = 1.0f;

	//CAMERA_T : Tiger's Eye
	pCamera = &camera_info[CAMERA_T];
	pCamera->move = 0, pCamera->fixed = CAMERA_NON_FIXED;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect;
	pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f, pCamera->zoom_factor = 1.5f;

	//CAMERA_G : Follow Tiger
	pCamera = &camera_info[CAMERA_G];
	pCamera->vup = glm::vec3(0.0f, 0.0f, 1.0f);
	pCamera->move = 0, pCamera->fixed = CAMERA_NON_FIXED;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect;
	pCamera->near_c = 0.1f; pCamera->far_c = 30000.0f, pCamera->zoom_factor = 1.5f;

	set_current_camera(CAMERA_M);
}
/*********************************  END: camera *********************************/

/******************************  START: shader setup ****************************/
// Begin of Callback function definitions
void prepare_shader_program(void) {
	char string[256];

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

	h_ShaderProgram_simple = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram_simple);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");

	h_ShaderProgram_TXPS = LoadShaders(shader_info_TXPS);
	loc_ModelViewProjectionMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrixInvTrans");

	loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_global_ambient_color");

	for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
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

	loc_texture = glGetUniformLocation(h_ShaderProgram_TXPS, "u_base_texture");
	loc_flag_texture_mapping = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_texture_mapping");
	loc_flag_fog = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_fog");
}
/*******************************  END: shder setup ******************************/

/****************************  START: geometry setup ****************************/
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))
#define INDEX_VERTEX_POSITION	0
#define INDEX_NORMAL			1
#define INDEX_TEX_COORD			2

bool b_draw_grid = false;

bool readTexImage2D_from_file(const char* filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP* tx_pixmap, * tx_pixmap_32;

	int width, height;
	GLvoid* data;

	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	if (tx_pixmap == NULL) {
		return false;
	}
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	//fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
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
	//fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap_32);
	if (tx_bits_per_pixel != 32)
		FreeImage_Unload(tx_pixmap);

	return true;
}

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

//bistro_exterior
GLuint* bistro_exterior_VBO;
GLuint* bistro_exterior_VAO;
int* bistro_exterior_n_triangles;
int* bistro_exterior_vertex_offset;
GLfloat** bistro_exterior_vertices;
GLuint* bistro_exterior_texture_names;

int flag_fog;
bool* flag_texture_mapping;

// for tiger animation
int flag_tiger_animation, flag_polygon_fill;
int cur_frame_tiger = 0, cur_frame_ben = 0, cur_frame_wolf, cur_frame_spider = 0;

void prepare_bistro_exterior(void) {
	int n_bytes_per_vertex, n_bytes_per_triangle;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	// VBO, VAO malloc
	bistro_exterior_VBO = (GLuint*)malloc(sizeof(GLuint) * scene.n_materials);
	bistro_exterior_VAO = (GLuint*)malloc(sizeof(GLuint) * scene.n_materials);

	bistro_exterior_n_triangles = (int*)malloc(sizeof(int) * scene.n_materials);
	bistro_exterior_vertex_offset = (int*)malloc(sizeof(int) * scene.n_materials);

	flag_texture_mapping = (bool*)malloc(sizeof(bool) * scene.n_textures);

	// vertices
	bistro_exterior_vertices = (GLfloat**)malloc(sizeof(GLfloat*) * scene.n_materials);

	for (int materialIdx = 0; materialIdx < scene.n_materials; materialIdx++) {
		MATERIAL* pMaterial = &(scene.material_list[materialIdx]);
		GEOMETRY_TRIANGULAR_MESH* tm = &(pMaterial->geometry.tm);

		// vertex
		bistro_exterior_vertices[materialIdx] = (GLfloat*)malloc(sizeof(GLfloat) * 8 * tm->n_triangle * 3);

		int vertexIdx = 0;
		for (int triIdx = 0; triIdx < tm->n_triangle; triIdx++) {
			TRIANGLE tri = tm->triangle_list[triIdx];
			for (int triVertex = 0; triVertex < 3; triVertex++) {
				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].x;
				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].y;
				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].z;

				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].x;
				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].y;
				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].z;

				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].u;
				bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].v;
			}
		}

		// # of triangles
		bistro_exterior_n_triangles[materialIdx] = tm->n_triangle;

		if (materialIdx == 0)
			bistro_exterior_vertex_offset[materialIdx] = 0;
		else
			bistro_exterior_vertex_offset[materialIdx] = bistro_exterior_vertex_offset[materialIdx - 1] + 3 * bistro_exterior_n_triangles[materialIdx - 1];

		glGenBuffers(1, &bistro_exterior_VBO[materialIdx]);

		glBindBuffer(GL_ARRAY_BUFFER, bistro_exterior_VBO[materialIdx]);
		glBufferData(GL_ARRAY_BUFFER, bistro_exterior_n_triangles[materialIdx] * 3 * n_bytes_per_vertex,
			bistro_exterior_vertices[materialIdx], GL_STATIC_DRAW);

		// As the geometry data exists now in graphics memory, ...
		free(bistro_exterior_vertices[materialIdx]);

		// Initialize vertex array object.
		glGenVertexArrays(1, &bistro_exterior_VAO[materialIdx]);
		glBindVertexArray(bistro_exterior_VAO[materialIdx]);

		glBindBuffer(GL_ARRAY_BUFFER, bistro_exterior_VBO[materialIdx]);
		glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
		glEnableVertexAttribArray(INDEX_VERTEX_POSITION);
		glVertexAttribPointer(INDEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
		glEnableVertexAttribArray(INDEX_NORMAL);
		glVertexAttribPointer(INDEX_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(6 * sizeof(float)));
		glEnableVertexAttribArray(INDEX_TEX_COORD);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		if ((materialIdx > 0) && (materialIdx % 100 == 0))
			fprintf(stdout, " * Loaded %d bistro exterior materials into graphics memory.\n", materialIdx / 100 * 100);
	}
	fprintf(stdout, " * Loaded %d bistro exterior materials into graphics memory.\n", scene.n_materials);

	// textures
	bistro_exterior_texture_names = (GLuint*)malloc(sizeof(GLuint) * scene.n_textures);
	glGenTextures(scene.n_textures, bistro_exterior_texture_names);

	for (int texId = 0; texId < scene.n_textures; texId++) {
		glActiveTexture(GL_TEXTURE0 + texId);
		glBindTexture(GL_TEXTURE_2D, bistro_exterior_texture_names[texId]);

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
	fprintf(stdout, " * Loaded bistro exterior textures into graphics memory.\n\n");
	
	free(bistro_exterior_vertices);
}

void draw_bistro_exterior(void) {
	glUseProgram(h_ShaderProgram_TXPS);
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
		glUniform4fv(loc_material.emissive_color, 1, scene.material_list[materialIdx].shading.ph.kr);

		int texId = scene.material_list[materialIdx].diffuseTexId;
		glUniform1i(loc_texture, texId);
		glUniform1i(loc_flag_texture_mapping, flag_texture_mapping[texId]);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0 + texId);
		glBindTexture(GL_TEXTURE_2D, bistro_exterior_texture_names[texId]);

		glBindVertexArray(bistro_exterior_VAO[materialIdx]);
		glDrawArrays(GL_TRIANGLES, 0, 3 * bistro_exterior_n_triangles[materialIdx]);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	}
	glUseProgram(0);
}

// tiger object
#define N_TIGER_FRAMES 12
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat* tiger_vertices[N_TIGER_FRAMES];

Material_Parameters material_tiger;
bool tiger_stop = false;
unsigned int tiger_timestamp_scene = 0; 

void prepare_tiger(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/tiger/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
			tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

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

	material_tiger.specular_color[0] = 0.728281f;
	material_tiger.specular_color[1] = 0.655802f;
	material_tiger.specular_color[2] = 0.466065f;
	material_tiger.specular_color[3] = 1.0f;

	material_tiger.specular_exponent = 51.2f;

	material_tiger.emissive_color[0] = 0.1f;
	material_tiger.emissive_color[1] = 0.1f;
	material_tiger.emissive_color[2] = 0.0f;
	material_tiger.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void set_material_tiger(void) {
	// assume ShaderProgram_TXPS is used
	glUniform4fv(loc_material.ambient_color, 1, material_tiger.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_tiger.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_tiger.specular_color);
	glUniform1f(loc_material.specular_exponent, material_tiger.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_tiger.emissive_color);
}

void draw_my_tiger_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	int tiger_time = tiger_timestamp_scene % 721;
	float tiger_sin_x, tiger_sin_y, tiger_sin_rotate, tiger_circle_rotate;

	if (tiger_time < 180) {
		tiger_sin_x = ((float)tiger_time - 90.0f) / 90.0f * 1519.2f;
		tiger_sin_y = 100.0f * sin(tiger_sin_x / 1519.2f * 360.0f * TO_RADIAN);
		tiger_sin_rotate = atan(100.0f / 1519.2f * 360.0f * TO_RADIAN * cos(tiger_sin_x / 1519.2f * 360.0f * TO_RADIAN));
	
		ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(2275.0f, -1686.65f, 0.0f));
		ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, -21.5f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix_Tiger = glm::translate(ModelMatrix_Tiger, glm::vec3(tiger_sin_x, tiger_sin_y, 0.0f));
		ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, tiger_sin_rotate + 90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else if (tiger_time >= 180 && tiger_time < 240) {
		ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(3700.0f, -2213.3f, 0.0f));
		ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, 1.588f + (float)(tiger_time - 180) / 60.0f * 1.204f, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else if (tiger_time >= 240 && tiger_time < 480) {
		tiger_circle_rotate = (float)(tiger_time - 240) * 360.0f / 240.0f + 20.0f;

		ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(4460.0f, -2500.0f, 0.0f));
		ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, -tiger_circle_rotate * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix_Tiger = glm::translate(ModelMatrix_Tiger, glm::vec3(-812.27f, 0.0f, 0.0f));
		ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else if (tiger_time >= 480 && tiger_time < 540) {
		ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(3700.0f, -2213.3f, 0.0f));
		ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, (160.0f + (float)(tiger_time - 480) / 60.0f * 88.5f) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else {
		ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f),
			glm::vec3(3700.0f - (float)(tiger_time - 540) / 180.0f * 2850.0f, -2213.3f + (float)(tiger_time - 540) / 180.0f * 1053.3f, 0.0f));
		ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, 248.5f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	ModelMatrix_Tiger = glm::scale(ModelMatrix_Tiger, glm::vec3(2.0f, 2.0f, 2.0f));
	ModelViewMatrix = ViewMatrix * ModelMatrix_Tiger;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);
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

	//material_ben.ambient_color[0] = 0.24725f;
	//material_ben.ambient_color[1] = 0.1995f;
	//material_ben.ambient_color[2] = 0.0745f;
	//material_ben.ambient_color[3] = 1.0f;
	//
	//material_ben.diffuse_color[0] = 0.75164f;
	//material_ben.diffuse_color[1] = 0.60648f;
	//material_ben.diffuse_color[2] = 0.22648f;
	//material_ben.diffuse_color[3] = 1.0f;
	//
	//material_ben.specular_color[0] = 0.728281f;
	//material_ben.specular_color[1] = 0.655802f;
	//material_ben.specular_color[2] = 0.466065f;
	//material_ben.specular_color[3] = 1.0f;
	//
	//material_ben.specular_exponent = 51.2f;
	//
	//material_ben.emissive_color[0] = 0.1f;
	//material_ben.emissive_color[1] = 0.1f;
	//material_ben.emissive_color[2] = 0.0f;
	//material_ben.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void draw_my_ben_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	int ben_time = timestamp_scene % 200;

	if (ben_time < 10) {
		ModelViewMatrix = glm::translate(ViewMatrix, 
			glm::vec3(-1.0f + (float)ben_time / 10.0f * 401.0f, 1290.0f + (float)ben_time / 10.0f * 410.0f, 20.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 150.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	else if (ben_time >= 10 && ben_time < 40) {
		ModelViewMatrix = glm::translate(ViewMatrix, 
			glm::vec3(400.0f + (float)(ben_time - 10.0f) / 30.0f * 1000.0f, 1700.0f + (float)(ben_time - 10.0f)/ 30.0f * 1700.0f, 10.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 160.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	else if (ben_time >=40 && ben_time < 60) {
		ModelViewMatrix = glm::translate(ViewMatrix,
			glm::vec3(1400.0f - (float)(ben_time - 40.0f) / 20.0f * 63.0f, 3400.0f + (float)(ben_time - 40.0f) / 20.0f * 900.0f,10.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	else if (ben_time >= 60 && ben_time < 65) { 
		ModelViewMatrix = glm::translate(ViewMatrix,
			glm::vec3(1337.0f + 40.0f * (ben_time - 60.0f), 4300.0f + 40.0f * (float)(ben_time - 60.0f), 40.0f * (float)(ben_time - 60.0f)));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 145.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, (90.0f - 18.0f * (float)(ben_time - 60)) * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	else if (ben_time >= 65 && ben_time < 100) {
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(1537.0f, 4500.0f, 200.0f + (float)(ben_time - 65.0f) / 35.0f * 450.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 145.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else if (ben_time >= 100 && ben_time < 120) {
		ModelViewMatrix = glm::translate(ViewMatrix, 
			glm::vec3(1537.0f + (float)(ben_time - 100.0f) / 20.0f * 423.0f, 4500.0f - (float)(ben_time - 100.0f) / 20.0f * 610.0f, 650.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 145.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (ben_time >= 120 && ben_time < 155) {
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(1960.0f, 3890.0f, 650.0f - (float)(ben_time - 120.0f) / 35.0f * 450.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 145.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -180.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else if (ben_time >= 155 && ben_time < 160) {
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(1960.0f - (float)(ben_time - 155.0f) / 5.0f * 116.0f, 
			3890.0f - (float)(ben_time - 155.0f) / 5.0f * 94.0f, 200.0f - (float)(ben_time - 155.0f) / 5.0f * 200.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 145.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -180.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -18.0f * (float)(ben_time - 135) * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	else {
		ModelViewMatrix = glm::translate(ViewMatrix,
			glm::vec3(1844.0f - (float)(ben_time - 160) / 40.0f * 1845.0f, 3796.0f - (float)(ben_time - 160) / 40.0f * 2506.0f, 10.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -35.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(250.0f, -250.0f, -250.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(ben_VAO);
	glDrawArrays(GL_TRIANGLES, ben_vertex_offset[cur_frame_ben], 3 * ben_n_triangles[cur_frame_ben]);
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

	//material_dragon.ambient_color[0] = 0.24725f;
	//material_dragon.ambient_color[1] = 0.1995f;
	//material_dragon.ambient_color[2] = 0.0745f;
	//material_dragon.ambient_color[3] = 1.0f;
	//
	//material_dragon.diffuse_color[0] = 0.75164f;
	//material_dragon.diffuse_color[1] = 0.60648f;
	//material_dragon.diffuse_color[2] = 0.22648f;
	//material_dragon.diffuse_color[3] = 1.0f;
	//
	//material_dragon.specular_color[0] = 0.728281f;
	//material_dragon.specular_color[1] = 0.655802f;
	//material_dragon.specular_color[2] = 0.466065f;
	//material_dragon.specular_color[3] = 1.0f;
	//
	//material_dragon.specular_exponent = 51.2f;
	//
	//material_dragon.emissive_color[0] = 0.1f;
	//material_dragon.emissive_color[1] = 0.1f;
	//material_dragon.emissive_color[2] = 0.0f;
	//material_dragon.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void draw_my_dragon_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	int dragon_time = timestamp_scene % 140;
	float dragon_sin_x, dragon_sin_z, dragon_sin_rotate;
	float dragon_tan_x, dragon_tan_z, dragon_tan_rotate;

	if (dragon_time < 100) {
		dragon_sin_x = ((float)dragon_time - 50.0f) / 50.0f * 677.7f;
		dragon_sin_z = 100.0f * sin((dragon_sin_x / 677.7f * 360.0f) * TO_RADIAN);
		dragon_sin_rotate = -atan(100.0f / 677.7f * 360.0f * TO_RADIAN * cos((dragon_sin_x / 677.7f * 360.0f) * TO_RADIAN));

		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-1850.0f, 1085.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 135.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(dragon_sin_x, 0.0f, 300.0f + dragon_sin_z));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, dragon_sin_rotate, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	else {
		dragon_tan_x = ((float)dragon_time - 100.0f) / 40.0f * 200.0f;
		dragon_tan_z = 300.0f * tan(dragon_tan_x / 200.0f * 90.0f * TO_RADIAN); 
		dragon_tan_rotate = 0.422371f - atan(300.0f / 200.0f * 90.0f * TO_RADIAN / (cos(dragon_tan_x / 200.0f * 90.0f * TO_RADIAN) * cos(dragon_tan_x / 200.0f * 90.0f * TO_RADIAN)));
	
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-2329.2f, 1564.2f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 135.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(dragon_tan_x, 0.0f, 300.0f + dragon_tan_z));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, dragon_tan_rotate, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(10.0f, 10.0f, 10.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(dragon_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * dragon_n_triangles);
	glBindVertexArray(0);
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

	//material_bike.ambient_color[0] = 0.24725f;
	//material_bike.ambient_color[1] = 0.1995f;
	//material_bike.ambient_color[2] = 0.0745f;
	//material_bike.ambient_color[3] = 1.0f;
	//
	//material_bike.diffuse_color[0] = 0.75164f;
	//material_bike.diffuse_color[1] = 0.60648f;
	//material_bike.diffuse_color[2] = 0.22648f;
	//material_bike.diffuse_color[3] = 1.0f;
	//
	//material_bike.specular_color[0] = 0.728281f;
	//material_bike.specular_color[1] = 0.655802f;
	//material_bike.specular_color[2] = 0.466065f;
	//material_bike.specular_color[3] = 1.0f;
	//
	//material_bike.specular_exponent = 51.2f;
	//
	//material_bike.emissive_color[0] = 0.1f;
	//material_bike.emissive_color[1] = 0.1f;
	//material_bike.emissive_color[2] = 0.0f;
	//material_bike.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void draw_my_bike_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-380.0f, -280.0f, 20.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 210.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(40.0f, 40.0f, 40.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(bike_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * bike_n_triangles);
	glBindVertexArray(0);
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

	//material_optimus.ambient_color[0] = 0.24725f;
	//material_optimus.ambient_color[1] = 0.1995f;
	//material_optimus.ambient_color[2] = 0.0745f;
	//material_optimus.ambient_color[3] = 1.0f;
	//
	//material_optimus.diffuse_color[0] = 0.75164f;
	//material_optimus.diffuse_color[1] = 0.60648f;
	//material_optimus.diffuse_color[2] = 0.22648f;
	//material_optimus.diffuse_color[3] = 1.0f;
	//
	//material_optimus.specular_color[0] = 0.728281f;
	//material_optimus.specular_color[1] = 0.655802f;
	//material_optimus.specular_color[2] = 0.466065f;
	//material_optimus.specular_color[3] = 1.0f;
	//
	//material_optimus.specular_exponent = 51.2f;
	//
	//material_optimus.emissive_color[0] = 0.1f;
	//material_optimus.emissive_color[1] = 0.1f;
	//material_optimus.emissive_color[2] = 0.0f;
	//material_optimus.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void draw_my_optimus_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(6300.0f, -3200.0f, 1715.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 160.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(1.5f, 1.5f, 1.5f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(optimus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * optimus_n_triangles);
	glBindVertexArray(0);
}

// godzilla object
GLuint godzilla_VBO, godzilla_VAO;
int godzilla_n_triangles;
GLfloat* godzilla_vertices;

Material_Parameters material_godzilla;

void prepare_godzilla(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, godzilla_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/godzilla_vnt.geom");
	godzilla_n_triangles = read_geometry(&godzilla_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	godzilla_n_total_triangles += godzilla_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &godzilla_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
	glBufferData(GL_ARRAY_BUFFER, godzilla_n_total_triangles * 3 * n_bytes_per_vertex, godzilla_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(godzilla_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &godzilla_VAO);
	glBindVertexArray(godzilla_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_godzilla.ambient_color[0] = 0.24725f;
	//material_godzilla.ambient_color[1] = 0.1995f;
	//material_godzilla.ambient_color[2] = 0.0745f;
	//material_godzilla.ambient_color[3] = 1.0f;
	//
	//material_godzilla.diffuse_color[0] = 0.75164f;
	//material_godzilla.diffuse_color[1] = 0.60648f;
	//material_godzilla.diffuse_color[2] = 0.22648f;
	//material_godzilla.diffuse_color[3] = 1.0f;
	//
	//material_godzilla.specular_color[0] = 0.728281f;
	//material_godzilla.specular_color[1] = 0.655802f;
	//material_godzilla.specular_color[2] = 0.466065f;
	//material_godzilla.specular_color[3] = 1.0f;
	//
	//material_godzilla.specular_exponent = 51.2f;
	//
	//material_godzilla.emissive_color[0] = 0.1f;
	//material_godzilla.emissive_color[1] = 0.1f;
	//material_godzilla.emissive_color[2] = 0.0f;
	//material_godzilla.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void draw_my_godzilla_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(5663.0f, -1960.0f, 35.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -60.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(1.5f, 1.5f, 1.5f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(godzilla_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * godzilla_n_triangles);
	glBindVertexArray(0);

	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(4945.0f, -3725.0f, 35.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -150.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(1.5f, 1.5f, 1.5f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(godzilla_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * godzilla_n_triangles);
	glBindVertexArray(0);
}

// ironman object
GLuint ironman_VBO, ironman_VAO;
int ironman_n_triangles;
GLfloat* ironman_vertices;

Material_Parameters material_ironman;

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

	//material_ironman.ambient_color[0] = 0.24725f;
	//material_ironman.ambient_color[1] = 0.1995f;
	//material_ironman.ambient_color[2] = 0.0745f;
	//material_ironman.ambient_color[3] = 1.0f;
	//
	//material_ironman.diffuse_color[0] = 0.75164f;
	//material_ironman.diffuse_color[1] = 0.60648f;
	//material_ironman.diffuse_color[2] = 0.22648f;
	//material_ironman.diffuse_color[3] = 1.0f;
	//
	//material_ironman.specular_color[0] = 0.728281f;
	//material_ironman.specular_color[1] = 0.655802f;
	//material_ironman.specular_color[2] = 0.466065f;
	//material_ironman.specular_color[3] = 1.0f;
	//
	//material_ironman.specular_exponent = 51.2f;
	//
	//material_ironman.emissive_color[0] = 0.1f;
	//material_ironman.emissive_color[1] = 0.1f;
	//material_ironman.emissive_color[2] = 0.0f;
	//material_ironman.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void draw_my_ironman_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-330.0f, -122.0f, 20.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -55.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(80.0f, 80.0f, 80.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(ironman_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
	glBindVertexArray(0);
}

// tank object
GLuint tank_VBO, tank_VAO;
int tank_n_triangles;
GLfloat* tank_vertices;

Material_Parameters material_tank;

void prepare_tank(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tank_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/tank_vnt.geom");
	tank_n_triangles = read_geometry(&tank_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	tank_n_total_triangles += tank_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &tank_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tank_VBO);
	glBufferData(GL_ARRAY_BUFFER, tank_n_total_triangles * 3 * n_bytes_per_vertex, tank_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(tank_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &tank_VAO);
	glBindVertexArray(tank_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tank_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_tank.ambient_color[0] = 0.24725f;
	//material_tank.ambient_color[1] = 0.1995f;
	//material_tank.ambient_color[2] = 0.0745f;
	//material_tank.ambient_color[3] = 1.0f;
	//
	//material_tank.diffuse_color[0] = 0.75164f;
	//material_tank.diffuse_color[1] = 0.60648f;
	//material_tank.diffuse_color[2] = 0.22648f;
	//material_tank.diffuse_color[3] = 1.0f;
	//
	//material_tank.specular_color[0] = 0.728281f;
	//material_tank.specular_color[1] = 0.655802f;
	//material_tank.specular_color[2] = 0.466065f;
	//material_tank.specular_color[3] = 1.0f;
	//
	//material_tank.specular_exponent = 51.2f;
	//
	//material_tank.emissive_color[0] = 0.1f;
	//material_tank.emissive_color[1] = 0.1f;
	//material_tank.emissive_color[2] = 0.0f;
	//material_tank.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	readTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void draw_my_tank_20192138(void) {
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-686.0f, 3382.0f, 20.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 135.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	glFrontFace(GL_CW);

	glBindVertexArray(tank_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * tank_n_triangles);
	glBindVertexArray(0);
}
/*****************************  END: geometry setup *****************************/

/********************  START: callback function definitions *********************/
void set_texture() {
	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void set_ViewMatrix_for_My_Tiger_20192138() {
	glm::mat4 Matrix_CAMERA_tiger_inverse;

	Matrix_CAMERA_tiger_inverse = ModelMatrix_Tiger * ModelMatrix_Tiger_Eye;
	ViewMatrix = glm::affineInverse(Matrix_CAMERA_tiger_inverse);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void set_ViewMatrix_for_Follow_My_Camera_20192138() {
	int tiger_Ptime, tiger_Vtime;
	glm::vec3 follow_coordinate;

	tiger_Vtime = timestamp_scene % 721;
	if (tiger_Vtime >= 180 && tiger_Vtime < 240) tiger_Ptime = 120;
	else if (tiger_Vtime >= 240 && tiger_Vtime < 300) tiger_Ptime = tiger_Vtime - 120;
	else if (tiger_Vtime >= 480 && tiger_Vtime < 540) tiger_Ptime = 420;
	else if (tiger_Vtime >= 540 && tiger_Vtime < 600) tiger_Ptime = tiger_Vtime - 120;
	else tiger_Ptime = (tiger_Vtime + 660) % 721;

	follow_coordinate = tiger_coordinate[tiger_Ptime] + glm::vec3(0.0f, 0.0f, 300.0f);
	ViewMatrix = glm::lookAt(follow_coordinate, tiger_coordinate[tiger_Vtime], current_camera.vup);
	ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy,
		current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (tiger_camera) {
		set_ViewMatrix_for_My_Tiger_20192138();
	}
	if (tiger_follow_camera) {
		set_ViewMatrix_for_Follow_My_Camera_20192138();
	}

	draw_grid();
	draw_axes();
	draw_bistro_exterior();

	// object에 texture mapping을 하기 위한 operation
	set_texture(); 

	// dynamic object : tiger, ben, dragon
	draw_my_tiger_20192138();
	draw_my_ben_20192138();
	draw_my_dragon_20192138();

	// static object : bike, optimus, godzilla, ironman, tank
	draw_my_bike_20192138();
	draw_my_optimus_20192138();
	draw_my_godzilla_20192138();
	draw_my_ironman_20192138();
	draw_my_tank_20192138();

	glUseProgram(0);
	glutSwapBuffers();
}

void timer_scene(int value) {
	timestamp_scene = (timestamp_scene + 1) % UINT_MAX;
	if (!tiger_stop) {
		tiger_timestamp_scene = (tiger_timestamp_scene + 1) % UINT_MAX;
		cur_frame_tiger = timestamp_scene % N_TIGER_FRAMES;
	}
	cur_frame_ben = timestamp_scene % N_BEN_FRAMES;
	glutPostRedisplay();
	if (flag_tiger_animation)
		glutTimerFunc(10, timer_scene, 0);
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'm':
		set_current_camera(CAMERA_M);
		glutPostRedisplay();
		break;
	case 's':
		if (tiger_stop == false) 
			tiger_stop = true;
		else 
			tiger_stop = false;
		break;
	case 'x':
		flag_translation_axis = X_AXIS;
		break;
	case 'y':
		flag_translation_axis = Y_AXIS;
		break;
	case 'z':
		flag_translation_axis = Z_AXIS;
		break;
	case 'u':
		flag_rotation_axis = U_AXIS;
		break;
	case 'v':
		flag_rotation_axis = V_AXIS;
		break;
	case 'n':
		flag_rotation_axis = N_AXIS;
		break;
	case 't':
		set_current_camera(CAMERA_T);
		glutPostRedisplay();
		break;
	case 'g':
		set_current_camera(CAMERA_G);
		glutPostRedisplay();
		break;
	case '0':
		set_current_camera(CAMERA_0);
		glutPostRedisplay();
		break;
	case '1':
		set_current_camera(CAMERA_1);
		glutPostRedisplay();
		break;
	case '2':
		set_current_camera(CAMERA_2);
		glutPostRedisplay();
		break;
	case '3':
		set_current_camera(CAMERA_3);
		glutPostRedisplay();
		break;
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}

void motion(int x, int y) {
	if (!current_camera.move) return;

	renew_cam_position(prevy - y);
	renew_cam_orientation_rotation_around_all_axis(prevx - x);

	prevx = x; prevy = y;

	set_ViewMatrix_from_camera_frame();
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	glutPostRedisplay();
}

#define CAM_ZOOM_STEP			0.05f
#define CAM_MAX_ZOOM_IN_FACTOR	0.25f
#define CAM_MAX_ZOOM_OUT_FACTOR	2.50f

void mouseWheel(int button, int dir, int x, int y) {
	if (tiger_camera || tiger_follow_camera)
		return;

	if (dir < 0 && (glutGetModifiers() == GLUT_ACTIVE_SHIFT)) {
		current_camera.zoom_factor += CAM_ZOOM_STEP;
		if (current_camera.zoom_factor > CAM_MAX_ZOOM_OUT_FACTOR)
			current_camera.zoom_factor = CAM_MAX_ZOOM_OUT_FACTOR;
		ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy,
			current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glutPostRedisplay();
	}
	else if (dir > 0 && (glutGetModifiers() == GLUT_ACTIVE_SHIFT)) {
		current_camera.zoom_factor -= CAM_ZOOM_STEP;
		if (current_camera.zoom_factor < CAM_MAX_ZOOM_IN_FACTOR)
			current_camera.zoom_factor = CAM_MAX_ZOOM_IN_FACTOR;
		ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy,
			current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glutPostRedisplay();
	}
	
}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON)) {
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

	current_camera.aspect_ratio = (float)width / height;
	ProjectionMatrix = glm::perspective(current_camera.zoom_factor * current_camera.fovy, 
		current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	glutPostRedisplay();
}

void cleanup(void) {
	glDeleteVertexArrays(1, &axes_VAO);
	glDeleteBuffers(1, &axes_VBO);

	glDeleteVertexArrays(1, &grid_VAO);
	glDeleteBuffers(1, &grid_VBO);

	glDeleteVertexArrays(scene.n_materials, bistro_exterior_VAO);
	glDeleteBuffers(scene.n_materials, bistro_exterior_VBO);

	glDeleteVertexArrays(1, &tiger_VAO);
	glDeleteBuffers(1, &tiger_VBO);

	glDeleteTextures(scene.n_textures, bistro_exterior_texture_names);
	glDeleteTextures(N_TEXTURES_USED, texture_names);

	free(bistro_exterior_n_triangles);
	free(bistro_exterior_vertex_offset);

	free(bistro_exterior_VAO);
	free(bistro_exterior_VBO);

	free(bistro_exterior_texture_names);
	free(flag_texture_mapping);
}
/*********************  END: callback function definitions **********************/

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutCloseFunc(cleanup);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutTimerFunc(100, timer_scene, 0);
	glutMouseWheelFunc(mouseWheel);
}

void initialize_lights(void) { // follow OpenGL conventions for initialization
	int i;

	glUseProgram(h_ShaderProgram_TXPS);

	glUniform4f(loc_global_ambient_color, 1.0f, 1.0f, 1.0f, 1.0f);

	for (i = 0; i < scene.n_lights; i++) {
		glUniform1i(loc_light[i].light_on, 1);
		glUniform4f(loc_light[i].position,
			scene.light_list[i].pos[0],
			scene.light_list[i].pos[1],
			scene.light_list[i].pos[2],
			0.0f);

		glUniform4f(loc_light[i].ambient_color, 0.13f, 0.13f, 0.13f, 1.0f);
		glUniform4f(loc_light[i].diffuse_color, 0.5f, 0.5f, 0.5f, 0.5f);
		glUniform4f(loc_light[i].specular_color, 0.8f, 0.8f, 0.8f, 1.0f);
		glUniform3f(loc_light[i].spot_direction, 0.0f, 0.0f, -1.0f);
		glUniform1f(loc_light[i].spot_exponent, 0.0f); // [0.0, 128.0]
		glUniform1f(loc_light[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
		glUniform4f(loc_light[i].light_attenuation_factors, 1.0f, 0.0f, 0.0f, 0.0f); // .w != 0.0f for no ligth attenuation
	}

	glUniform4f(loc_material.ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
	glUniform4f(loc_material.diffuse_color, 0.8f, 0.8f, 0.8f, 1.0f);
	glUniform4f(loc_material.specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform4f(loc_material.emissive_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform1f(loc_material.specular_exponent, 0.0f); // [0.0, 128.0]

	glUseProgram(0);
}

void initialize_flags(void) {
	flag_fog = 0;
	flag_tiger_animation = 1;
	flag_polygon_fill = 1;

	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_fog, flag_fog);
	glUniform1i(loc_flag_texture_mapping, 1);
	glUseProgram(0);
}

void set_Tiger_Camera() {
	int tiger_time;
	float tiger_sin_x, tiger_sin_y, tiger_sin_rotate, tiger_circle_rotate;

	tiger_sin_x = -1519.2f;
	tiger_sin_y = 100.0f * sin(tiger_sin_x / 1519.2f * 360.0f * TO_RADIAN);
	tiger_sin_rotate = atan(100.0f / 1519.2f * 360.0f * TO_RADIAN * cos(tiger_sin_x / 1519.2f * 360.0f * TO_RADIAN));

	ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(2275.0f, -1686.65f, 0.0f));
	ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, -21.5f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix_Tiger = glm::translate(ModelMatrix_Tiger, glm::vec3(tiger_sin_x, tiger_sin_y, 0.0f));
	ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, tiger_sin_rotate + 90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix_Tiger = glm::scale(ModelMatrix_Tiger, glm::vec3(2.0f, 2.0f, 2.0f));

	ModelMatrix_Tiger_Eye = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -71.5f, 79.0f));
	ModelMatrix_Tiger_Eye = glm::rotate(ModelMatrix_Tiger_Eye, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix_Tiger_Eye = glm::rotate(ModelMatrix_Tiger_Eye, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));

	for (tiger_time = 0; tiger_time <= 720; tiger_time++) {
		if (tiger_time < 180) {
			tiger_sin_x = ((float)tiger_time - 90.0f) / 90.0f * 1519.2f;
			tiger_sin_y = 100.0f * sin(tiger_sin_x / 1519.2f * 360.0f * TO_RADIAN);
			tiger_sin_rotate = atan(100.0f / 1519.2f * 360.0f * TO_RADIAN * cos(tiger_sin_x / 1519.2f * 360.0f * TO_RADIAN));

			ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(2275.0f, -1686.65f, 0.0f));
			ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, -21.5f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
			ModelMatrix_Tiger = glm::translate(ModelMatrix_Tiger, glm::vec3(tiger_sin_x, tiger_sin_y, 0.0f));
			ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, tiger_sin_rotate + 90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else if (tiger_time >= 180 && tiger_time < 240) {
			ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(3700.0f, -2213.3f, 0.0f));
			ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, 1.588f + (float)(tiger_time - 180) / 60.0f * 1.204f, glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else if (tiger_time >= 240 && tiger_time < 480) {
			tiger_circle_rotate = (float)(tiger_time - 240) * 360.0f / 240.0f + 20.0f;

			ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(4460.0f, -2500.0f, 0.0f));
			ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, -tiger_circle_rotate * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
			ModelMatrix_Tiger = glm::translate(ModelMatrix_Tiger, glm::vec3(-812.27f, 0.0f, 0.0f));
			ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else if (tiger_time >= 480 && tiger_time < 540) {
			ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f), glm::vec3(3700.0f, -2213.3f, 0.0f));
			ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, (160.0f + (float)(tiger_time - 480) / 60.0f * 88.5f) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else {
			ModelMatrix_Tiger = glm::translate(glm::mat4(1.0f),
				glm::vec3(3700.0f - (float)(tiger_time - 540) / 180.0f * 2850.0f, -2213.3f + (float)(tiger_time - 540) / 180.0f * 1053.3f, 0.0f));
			ModelMatrix_Tiger = glm::rotate(ModelMatrix_Tiger, 248.5f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		}
		ModelMatrix_Tiger = glm::scale(ModelMatrix_Tiger, glm::vec3(2.0f, 2.0f, 2.0f));
		tiger_coordinate[tiger_time] = glm::vec3(ModelMatrix_Tiger[3]);
	}
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
	flag_translation_axis = Z_AXIS;
	flag_rotation_axis = V_AXIS;
	set_Tiger_Camera();

	glGenTextures(N_TEXTURES_USED, texture_names);
}

void set_up_scene_lights(void) {
	// point_light_EC: use light 0
	light[0].light_on = 1;
	light[0].position[0] = 0.0f; light[0].position[1] = 100.0f; 	// point light position in EC
	light[0].position[2] = 0.0f; light[0].position[3] = 1.0f;

	light[0].ambient_color[0] = 0.13f; light[0].ambient_color[1] = 0.13f;
	light[0].ambient_color[2] = 0.13f; light[0].ambient_color[3] = 1.0f;

	light[0].diffuse_color[0] = 0.5f; light[0].diffuse_color[1] = 0.5f;
	light[0].diffuse_color[2] = 0.5f; light[0].diffuse_color[3] = 1.5f;

	light[0].specular_color[0] = 0.8f; light[0].specular_color[1] = 0.8f;
	light[0].specular_color[2] = 0.8f; light[0].specular_color[3] = 1.0f;

	// spot_light_WC: use light 1
	light[1].light_on = 1;
	light[1].position[0] = 0.0f; light[1].position[1] = 0.0f; // spot light position in WC
	light[1].position[2] = 0.0f; light[1].position[3] = 1.0f;

	light[1].ambient_color[0] = 0.152f; light[1].ambient_color[1] = 0.152f;
	light[1].ambient_color[2] = 0.152f; light[1].ambient_color[3] = 1.0f;

	light[1].diffuse_color[0] = 0.572f; light[1].diffuse_color[1] = 0.572f;
	light[1].diffuse_color[2] = 0.572f; light[1].diffuse_color[3] = 1.0f;

	light[1].specular_color[0] = 0.772f; light[1].specular_color[1] = 0.772f;
	light[1].specular_color[2] = 0.772f; light[1].specular_color[3] = 1.0f;

	light[1].spot_direction[0] = 0.0f; light[1].spot_direction[1] = -1.0f; // spot light direction in WC
	light[1].spot_direction[2] = 0.0f;
	light[1].spot_cutoff_angle = 20.0f;
	light[1].spot_exponent = 8.0f;

	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_light[0].light_on, light[0].light_on);
	glUniform4fv(loc_light[0].position, 1, light[0].position);
	glUniform4fv(loc_light[0].ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_light[0].diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_light[0].specular_color, 1, light[0].specular_color);

	glUniform1i(loc_light[1].light_on, light[1].light_on);
	
	// need to supply position in EC for shading
	glm::vec4 position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
		light[1].position[2], light[1].position[3]);
	glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
	glUniform4fv(loc_light[1].ambient_color, 1, light[1].ambient_color);
	glUniform4fv(loc_light[1].diffuse_color, 1, light[1].diffuse_color);
	glUniform4fv(loc_light[1].specular_color, 1, light[1].specular_color);
	
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0], light[1].spot_direction[1],
		light[1].spot_direction[2]);
	glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
	glUniform1f(loc_light[1].spot_cutoff_angle, light[1].spot_cutoff_angle);
	glUniform1f(loc_light[1].spot_exponent, light[1].spot_exponent);
	glUseProgram(0);
}

void prepare_scene(void) {
	prepare_axes();
	prepare_grid();
	prepare_bistro_exterior();
	prepare_tiger();
	prepare_ben();
	prepare_dragon();
	prepare_bike();
	prepare_optimus();
	prepare_godzilla();
	prepare_ironman();
	prepare_tank();
	set_up_scene_lights();
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
	char program_name[64] = "Sogang CSE4170 Bistro Exterior Scene";
	char messages[N_MESSAGE_LINES][256] = { 
		"    - Keys used:",
		"		'0' : set the camera for tiger view(1)",
		"		'1' : set the camera for tiger view(2)",
		"		'2' : set the camera for dragon view",
		"		'3' : set the camera for ben view",
		"		'm' : set the moving camera for world",
		"		't' : set the moving camera for tiger's eye",
		"		's' : set the tiger stop or move",
		"		'x' : set the x-axis direction",
		"		'y' : set the y-axis direction",	
		"		'z' : set the z-axis direction",
		"		'u' : set the u-axis rotation",
		"		'v' : set the v-axis rotation",
		"		'n' : set the n-axis rotation",
		"		'ESC' : program close",
	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
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
