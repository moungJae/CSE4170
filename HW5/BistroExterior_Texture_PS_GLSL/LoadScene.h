//
//  LoadScene.h
//
//  Written for CSE4170
//  Department of Computer Science and Engineering
//  Copyright © 2022 Sogang University. All rights reserved.
//

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeImage/FreeImage.h>

#define INVALID_TEX_ID		(0xffffffff)
#define MAX_TEXTURE_FILES	(1024)

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

typedef struct {
	float e[3];
	float at[3];
	float vup[3];
	float u[3], v[3], n[3]; // three orthonormal vectors for camera frame
	float fovy, aspect;
} CAMERA;

typedef enum {
	GEOMETRY_TYPE_SPHERE = 0,
	GEOMETRY_TYPE_RECTANGLE = 1,
	GEOMETRY_TYPE_PARALLELOGRAM = 2,
	GEOMETRY_TYPE_AABB = 3,
	GEOMETRY_TYPE_TRIANGULAR_MESH = 4,
	GEOMETRY_TYPE_SPHERE_SHELL = 5,
	GEOMETRY_TYPE_COUNT
} GEOMETRY_TYPE;

typedef struct {
	float center[3];
	float radius;
} GEOMETRY_SPHERE;

typedef struct {
	float anchor[3], u[3], v[3];  // TODO: check if u_dot_v = 0
	float u_dot_u, v_dot_v; // to be computed from anchor, u, and v
	float norm[3], anchor_dot_norm; // to be computed from anchor, u, and v
} GEOMETRY_RECTANGLE;

typedef struct {
	float anchor[3], u[3], v[3];
	float u_dot_u, v_dot_v; // to be computed from anchor, u, and v
	float norm[3], anchor_dot_norm; // to be computed from anchor, u, and v
} GEOMETRY_PARALLELOGRAM;

typedef enum {
	TRI_VERTEX_1,
	TRI_VERTEX_2,
	TRI_VERTEX_3,
	NUM_TRI_VERTICES,
} TRI_VERTEX;

typedef struct {
	float x;
	float y;
	float z;
} float3;

typedef struct {
	float u;
	float v;
} float2;

typedef struct {
	float3 p_min;
	float3 p_max;
} GEOMETRY_AABB;

typedef struct {
	float3 e1, e2;
	float3 e2e1;
}TRIACCEL;

typedef struct {
	float3	position[NUM_TRI_VERTICES];
	float3	normal_vetcor[NUM_TRI_VERTICES];
	float2* texture_list[NUM_TRI_VERTICES];
	TRIACCEL accel;
	float3 tangent;
	float3 bitangent;
} TRIANGLE;

typedef struct {
	int				n_triangle;
	TRIANGLE*		triangle_list;
	int				object_id;
	int				n_textures;
	GEOMETRY_AABB	aabb;
} GEOMETRY_TRIANGULAR_MESH;

typedef struct {
	float center[3];
	float radius_in, radius_out;
} GEOMETRY_SPHERE_SHELL;

typedef enum {
	SHADING_TYPE_PHONG = 0,
	SHADING_TYPE_PHONG_CHECKER = 1,
	SHADING_TYPE_PHONG_TEXTURE = 2,
	SHADING_TYPE_GLASS = 3,
	SHADING_TYPE_COUNT
} SHADING_TYPE;

typedef struct {  // Opaque material
	float ka[3], kd[3], ks[3], kr[3];
	float spec_exp;

	//new
	float opacity;
	float reflectivity;
} SHADING_PHONG;

typedef struct {  // Opaque material
	float ka[2][3], kd[2][3], ks[2][3], kr[2][3];
	float spec_exp[2];
	int checker_frequency[2];
} SHADING_PHONG_CHECKER;

typedef struct {  // Opaque material
	float ka[3], kd[3], ks[3], kr[3];
	float spec_exp;
	unsigned int tex_unit;
} SHADING_PHONG_TEXTURE;

typedef struct {  // Not implemented yet
	float ka[3], kd[3], ks[3], kr[3];
	float spec_exp;
} SHADING_GLASS;

typedef struct {
	GEOMETRY_TYPE geometry_type;
	union {
		GEOMETRY_SPHERE sp;
		GEOMETRY_RECTANGLE re;
		GEOMETRY_PARALLELOGRAM pa;
		GEOMETRY_AABB ab;
		GEOMETRY_TRIANGULAR_MESH tm;
		GEOMETRY_SPHERE_SHELL ss;
	} geometry;

	SHADING_TYPE shading_type;
	union {
		SHADING_PHONG ph;
		SHADING_PHONG_CHECKER pc;
		SHADING_PHONG_TEXTURE pt;
		SHADING_GLASS gl;
	} shading;

	int diffuseTexId;
	int normalMapTexId;
	int specularTexId;
	int emissiveTexId;
} MATERIAL;

typedef enum {
	LIGHT_POINT,
	LIGHT_DIRECTIONAL,
	LIGHT_SPOT,
	LIGHT_AREA,
	LIGHT_VOLUME,
} LIGHT_TYPE;

typedef struct {
	LIGHT_TYPE	type;
	float		pos[4];
	float		color[3];
	float		intensitiy;
	float		fog;

	float		light_attenuation_factors[3];
	float		spot_cutoff_angle;
	float		spot_exp;
	float		spot_dir[3];
} LIGHT;

typedef struct {
	CAMERA			camera;
	float			ambient_light_color[3];
	float			background_color[3];
	int				n_materials;
	MATERIAL*		material_list;
	int				n_lights;
	LIGHT*			light_list;
	int				n_textures;
	char			texture_file_name[MAX_TEXTURE_FILES][256];
} SCENE;

// LoadScene.cpp
void read_3D_scene_from_file(SCENE* pScene);
void freeData(SCENE* pScene);
