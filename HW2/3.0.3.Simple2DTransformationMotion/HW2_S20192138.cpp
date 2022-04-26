#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

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
float centerx = 0.0f, centery = 0.0f;

// 2D 물체 정의 부분은 objects.h 파일로 분리
// 새로운 물체 추가 시 prepare_scene() 함수에서 해당 물체에 대한 prepare_***() 함수를 수행함.
// (필수는 아니나 올바른 코딩을 위하여) cleanup() 함수에서 해당 resource를 free 시킴.
#include "objects.h"

int leftbuttonpressed = 0;
int animation_mode = 1;
unsigned int timestamp = 0;

void timer(int value) {
	timestamp = (timestamp + 1) % UINT_MAX;
	glutPostRedisplay();
	if (animation_mode)
		glutTimerFunc(6, timer, 0);
}

void display(void) {
	glm::mat4 ModelMatrix;
	
	glClear(GL_COLOR_BUFFER_BIT);

	int sword_all_rotate = (timestamp + 75) % 720;
	int sword_circle = (timestamp + 75) % 1080;
	int sword_clock = timestamp % 90;
	float sword_centerX, sword_centerY;
	float sword_rotate;

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 1; k <= 3; k++) {
				sword_centerX = (float)k * 125.0f * pow(cos((sword_clock + 30 * k) * TO_RADIAN), 3);
				sword_centerY = (float)k * 125.0f * pow(sin((sword_clock + 30 * k) * TO_RADIAN), 3);
				sword_rotate = 90.0f * TO_RADIAN + atan(-tan((sword_clock + 30 * k) * TO_RADIAN));

				if (k % 2) {
					ModelMatrix = glm::rotate(glm::mat4(1.0f), sword_circle / 3 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
					ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-100.0f, 0, 0.0f));
				}
				else {
					ModelMatrix = glm::rotate(glm::mat4(1.0f), sword_circle / 3 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
					ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-100.0f, 0, 0.0f));
					ModelMatrix = glm::scale(ModelMatrix, glm::vec3(-1.0f, 1.0f, 1.0f));
				}
				ModelMatrix = glm::rotate(ModelMatrix, (float)i * (45.0f) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
				ModelMatrix = glm::rotate(ModelMatrix, (float)j * (90.0f) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
				ModelMatrix = glm::translate(ModelMatrix, glm::vec3(sword_centerX, sword_centerY, 0.0f));
				ModelMatrix = glm::rotate(ModelMatrix, sword_rotate, glm::vec3(0.0f, 0.0f, 1.0f));
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f, 3.0f, 1.0f));

				ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
				glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
				draw_sword();
			}
		}
	}

	int shirt_clock = (timestamp + 15) % 360;
	int shirt_centerX, shirt_centerY;

	if (shirt_clock >= 0 && shirt_clock < 45) {
		shirt_centerX = win_width / 4;
		shirt_centerY = -win_height * shirt_clock / 180;
	}
	else if (shirt_clock >= 45 && shirt_clock < 90) {
		shirt_centerX = win_width / 4;
		shirt_centerY = -win_height * (90 - shirt_clock) / 180;
	}
	else if (shirt_clock >= 90 && shirt_clock < 135) {
		shirt_centerX = win_width / 4;
		shirt_centerY = win_height * (shirt_clock - 90) / 180;
	}
	else if (shirt_clock >= 135 && shirt_clock < 180) {
		shirt_centerX = win_width / 4;
		shirt_centerY = win_height * (180 - shirt_clock) / 180;
	}
	else {
		shirt_centerX = win_width / 4;
		shirt_centerY = 0;
	}

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(shirt_centerX, shirt_centerY, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 2 * shirt_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-SHIRT_ROTATION_RADIUS, 0.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f, 3.0f, 1.0f));

	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_shirt();

	int cake_all_rotate = (timestamp + 30) % 360;
	int cake_time = (timestamp + 30) % 90; 
	float cake_rotate[4] = {0.0f, 180.0f, 270.0f, 90.0f};
	float cake_centerX[4], cake_centerY[4];
	float cake_scale_size[4];

	if ((timestamp + 30) % 360 >= 180)
		cake_all_rotate = -cake_all_rotate;

	for (int i = 0; i < 4; i++) {
		if (i < 2) {
			cake_centerX[i] = 0.0f;
			if (cake_time >= 0 && cake_time < 45) {
				cake_centerY[i] = 200.0f * ((45.0f - (float)cake_time) / 45.0f);
				cake_scale_size[i] = 1.0f + 6.0f * ((float)cake_time / 45.0f);
			}
			else {
				cake_centerY[i] = 200.0f * (((float)cake_time - 45.0f) / 45.0f);
				cake_scale_size[i] = 13.0f - 6.0f * ((float)cake_time / 45.0f);
			}
			if (i == 1) cake_centerY[i] = -cake_centerY[i];
		}
		else {
			cake_centerY[i] = 0.0f;
			if (cake_time >= 0 && cake_time < 45) {
				cake_centerX[i] = 200.0f * ((float)cake_time / 45.0f);
				cake_scale_size[i] = 7.0f - 6.0f * ((float)cake_time / 45.0f);
			}
			else {
				cake_centerX[i] = 200.0f * ((90.0f - (float)cake_time) / 45.0f);
				cake_scale_size[i] = -5.0f + 6.0f * ((float)cake_time / 45.0f);
			}
			if (i == 3) cake_centerX[i] = -cake_centerX[i];
		}

		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-win_width / 4, win_height / 4, 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, -(float)cake_all_rotate * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(cake_centerX[i], cake_centerY[i], 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, cake_rotate[i] * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(cake_scale_size[i], cake_scale_size[i], 1.0f));

		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_cake();
	}

	int car_time = (timestamp + 45) % 2885;
	
	if (car_time < 1440) {
		for (int i = car_time; i >= 0; i -= 75) {
			
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-win_width / 4, -win_height / 4, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(-1.0f, 1.0f, 1.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(300.0f * cos(i * TO_RADIAN) * pow(1.2f, -(i * TO_RADIAN)),
				300.0f * sin(i * TO_RADIAN) * pow(1.2f, -(i * TO_RADIAN)), 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, (-90 + i) * TO_RADIAN , glm::vec3(0.0f, 0.0f, 1.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f * pow(1.1f, -(i * TO_RADIAN)), 3.0f * pow(1.1f, -(i * TO_RADIAN)), 1.0f));

			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_car2();
		}
	}
	else {
		for (int i = car_time; i >= car_time - 1440; i -= 75) {
			if (i >= 1440) 
				continue;
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-win_width / 4, -win_height / 4, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(-1.0f, 1.0f, 1.0f));
			ModelMatrix = glm::translate(ModelMatrix, glm::vec3(300.0f * cos(i * TO_RADIAN) * pow(1.2f, -(i * TO_RADIAN)),
				300.0f * sin(i * TO_RADIAN) * pow(1.2f, -(i * TO_RADIAN)), 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, (-90 + i) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f * pow(1.1f, -(i * TO_RADIAN)), 3.0f * pow(1.1f, -(i * TO_RADIAN)), 1.0f));

			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_car2();
		}
	}

	int airplane_clock = (timestamp % 1442) / 2 - 360;
	float airplane_alpha;

	if (abs(airplane_clock) >= 0 && abs(airplane_clock) < 120)
		airplane_alpha = 50.0f;
	else if (abs(airplane_clock) >= 120 && abs(airplane_clock) < 240)
		airplane_alpha = 100.0f;
	else 
		airplane_alpha = 200.0f;

	float rotation_angle_airplane 
		= atanf(airplane_alpha * 3 * TO_RADIAN * cosf(3 * airplane_clock * TO_RADIAN));

	ModelMatrix = glm::rotate(glm::mat4(1.0f), 90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3((float)airplane_clock,
		airplane_alpha * sinf(3 * airplane_clock * TO_RADIAN), 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_airplane, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::rotate(ModelMatrix, 90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));

	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_airplane(); 

	int cocktail_clock = (timestamp + 60) % 1440;
	float cocktail_centerX, cocktail_centerY;
	float cocktail_rotate;

	if ((cocktail_clock % 360) >= 0 && (cocktail_clock % 360) < 180) {
		float tmp = cocktail_clock % 360;
		cocktail_rotate = (1440.0f / (180.0f * 180.0f)) * tmp * tmp;
	}
	else {
		float tmp = cocktail_clock % 360 - 360.0f;
		cocktail_rotate = (1440.0f / (180.0f * 180.0f)) * tmp * tmp;
	}

	if ((cocktail_clock / 360) == 0) {
		cocktail_centerX = ((float)(win_width) / (3.0f * sqrt(720))) * sqrt(cocktail_clock);
		cocktail_centerY = -((float)(win_height) / (3.0f * sqrt(360))) * sqrt(cocktail_clock);
	}
	else if ((cocktail_clock / 360) == 1) {
		cocktail_centerX = ((float)(win_width) / (3.0f * sqrt(720))) * sqrt(cocktail_clock);
		cocktail_centerY = -((float)(win_height) / (3.0f * sqrt(360))) * sqrt(720.0f - cocktail_clock);
	}
	else if ((cocktail_clock / 360) == 2) {
		cocktail_centerX = ((float)(win_width) / (3.0f * sqrt(720))) * sqrt(1440.0f - cocktail_clock);
		cocktail_centerY = ((float)(win_height) / (3.0f * sqrt(360))) * sqrt(cocktail_clock - 720.0f);
	}
	else {
		cocktail_centerX = ((float)(win_width) / (3.0f * sqrt(720))) * sqrt(1440.0f - cocktail_clock);
		cocktail_centerY = ((float)(win_height) / (3.0f * sqrt(360))) * sqrt(1440.0f - cocktail_clock);
	}

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(cocktail_centerX, cocktail_centerY, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, cocktail_rotate * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.5f, 2.5f, 1.0f));

	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_cocktail();

	glFlush();	
}   

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'a':
		animation_mode = 1 - animation_mode;
		if (animation_mode)
			glutTimerFunc(10, timer, 0);
		break;
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
		leftbuttonpressed = 1;
	else if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP))
		leftbuttonpressed = 0;
}

void motion(int x, int y) {
	if (leftbuttonpressed) {
		centerx =  x - win_width/2.0f, centery = (win_height - y) - win_height/2.0f;
		glutPostRedisplay();
	}
} 
	
void reshape(int width, int height) {
	win_width = width, win_height = height;
	
  	glViewport(0, 0, win_width, win_height);
	ProjectionMatrix = glm::ortho(-win_width / 2.0, win_width / 2.0, 
		-win_height / 2.0, win_height / 2.0, -1000.0, 1000.0);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	update_axes();

	glutPostRedisplay();
}

void cleanup(void) {
	glDeleteVertexArrays(1, &VAO_axes);
	glDeleteBuffers(1, &VBO_axes);

	glDeleteVertexArrays(1, &VAO_airplane);
	glDeleteBuffers(1, &VBO_airplane);

	glDeleteVertexArrays(1, &VAO_shirt);
	glDeleteBuffers(1, &VBO_shirt);

	glDeleteVertexArrays(1, &VAO_car2);
	glDeleteBuffers(1, &VBO_car2);

	glDeleteVertexArrays(1, &VAO_cocktail);
	glDeleteBuffers(1, &VBO_cocktail);

	glDeleteVertexArrays(1, &VAO_cake);
	glDeleteBuffers(1, &VBO_cake);

	glDeleteVertexArrays(1, &VAO_sword);
	glDeleteBuffers(1, &VBO_sword);
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutReshapeFunc(reshape);
	glutTimerFunc(10, timer, 0); // animation_mode = 1
	glutCloseFunc(cleanup);
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
	
	glClearColor(44 / 255.0f, 180 / 255.0f, 49 / 255.0f, 1.0f);
	ViewMatrix = glm::mat4(1.0f);
}

void prepare_scene(void) {
	prepare_axes();
	prepare_airplane(); 
	prepare_shirt();
	prepare_car2();
	prepare_cocktail();
	prepare_cake();
	prepare_sword();
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
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 2
int main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 HW2";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'a', 'ESC'"
		"    - Mouse used: L-click and move"
	};

	glutInit (&argc, argv);
 	glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize (1200, 800);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}


