//
//  LoadScene.cpp
//
//  Written for CSE4170
//  Department of Computer Science and Engineering
//  Copyright © 2022 Sogang University. All rights reserved.
//

#define _CRT_SECURE_NO_WARNINGS 

#include "LoadScene.h"

void read_3D_scene_from_file(SCENE* pScene) {
	FILE* fp = fopen("./Scene/BistroExterior.bin", "rb");
	fread(pScene, sizeof(SCENE), 1, fp);

	//light list save
	pScene->light_list = (LIGHT*)malloc(sizeof(LIGHT) * pScene->n_lights);
	fread(pScene->light_list, sizeof(LIGHT), pScene->n_lights, fp);

	//material list save
	pScene->material_list = (MATERIAL*)malloc(sizeof(MATERIAL) * pScene->n_materials);
	fread(pScene->material_list, sizeof(MATERIAL), pScene->n_materials, fp);
	for (int materialIdx = 0; materialIdx < pScene->n_materials; materialIdx++) {
		MATERIAL* pMaterial = &(pScene->material_list[materialIdx]);

		//triangle list save
		GEOMETRY_TRIANGULAR_MESH* pMesh = &(pMaterial->geometry.tm);
		pMesh->triangle_list = (TRIANGLE*)malloc(sizeof(TRIANGLE) * pMesh->n_triangle);
		fread(pMesh->triangle_list, sizeof(TRIANGLE), pMesh->n_triangle, fp);

		for (int triIdx = 0; triIdx < pMesh->n_triangle; triIdx++)
		{
			TRIANGLE* triObj = &(pMesh->triangle_list[triIdx]);
			for (int vertexIdx = 0; vertexIdx < NUM_TRI_VERTICES; vertexIdx++)
			{
				triObj->texture_list[vertexIdx] = (float2*)malloc(sizeof(float2) * pMesh->n_textures);
				fread(triObj->texture_list[vertexIdx], sizeof(float2), pMesh->n_textures, fp);
			}
		}
	}

	fclose(fp);
}

void freeData(SCENE* pScene) {
	free(pScene->light_list);

	for (int materialIdx = 0; materialIdx < pScene->n_materials; materialIdx++) {
		MATERIAL* pMaterial = &(pScene->material_list[materialIdx]);

		GEOMETRY_TRIANGULAR_MESH* pMesh = &(pMaterial->geometry.tm);
		for (int triIdx = 0; triIdx < pMesh->n_triangle; triIdx++) {
			TRIANGLE* tri = &(pMesh->triangle_list[triIdx]);

			for (int vertex = 0; vertex < NUM_TRI_VERTICES; vertex++)
				free(tri->texture_list[vertex]);
		}
		free(pMesh->triangle_list);
	}

	free(pScene->material_list);
}