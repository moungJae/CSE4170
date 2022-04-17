#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

// HW1 : GLUT 툴킷을 사용한 간단한 다각형 편집 기능 구현
// 20192138 조명재 

typedef struct point {
	float x;
	float y;
} Point;

int window_width = 0, window_height = 0; // To be used later.
int prev_x = 0, prev_y = 0; // 이전 좌표에 대한 정보
int rightbuttonpressed = 0; // 오른쪽 버튼을 클릭했는지에 대한 정보
int animation_mode = 0; // 30도씩 회전하는 경우 1, 아닐 경우 0
unsigned int timestamp = 0; // timer 함수에서 필요한 변수
float r = 0.0f, g = 1.0f, b = 0.0f; // 배경색은 초록색으로
Point center; // 다각형의 무게중심에 대한 정보
Point p[128];
int p_size = 0; // 점들의 갯수
int p_click = 0; // 다각형이 만들어졌는지 확인하는 용도 

void timer(int value) {
	float next_x, next_y;
	// 회전 각도는 30도로 정하였다.
	// cos(30) = 0.866 (대략적으로), sin(30) = 0.5 이다.
	// 회전변환을 이용하여 점들의 위치를 갱신하였다.
	for (int i = 0; i < p_size; i++) {
		next_x = center.x + (p[i].x - center.x) * (0.866) - (p[i].y - center.y) * (0.5);
		next_y = center.y + (p[i].x - center.x) * (0.5) + (p[i].y - center.y) * (0.866);
		p[i].x = next_x, p[i].y = next_y;
	}
	timestamp = (timestamp + 1) % UINT_MAX;
	glutPostRedisplay();
	if (animation_mode)
		glutTimerFunc(500, timer, 0);
}

void display(void) {
	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// 화면에 클릭한 점 그리기 (파란색)
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glColor3f(0.0f, 0.0f, 1.0f); // 점은 모두 파란색으로 지정
	center.x = center.y = 0.0f;
	for (int i = 0; i < p_size; i++) {
		glVertex2f(p[i].x, p[i].y);
		center.x += p[i].x;
		center.y += p[i].y;
	}
	// Null Pointer Exception 을 방지해야 한다.
	// 따라서 점이 적어도 한 개 존재할 경우 다각형의 무게중심을 구한다.
	if (p_size) { 
		center.x /= p_size;
		center.y /= p_size;
	}
	glEnd();

	// 화면에 클릭한 점들을 선으로 이어주기 (빨간색)
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	for (int i = 0; i < p_size - 1; i++) {
		glVertex2f(p[i].x, p[i].y); 
		glVertex2f(p[i + 1].x, p[i + 1].y);
	}
	// 'p' 키를 입력하면, 다각형이 그려지도록 첫 점과 가장 마지막 점을 연결
	if (p_click) {
		glVertex2f(p[0].x, p[0].y);
		glVertex2f(p[p_size - 1].x, p[p_size - 1].y);
	}
	glEnd();

	// 무게 중심 (노란색)
	if (animation_mode) {
		glPointSize(5.0f);
		glBegin(GL_POINTS);
		glColor3f(1.0f, 1.0f, 0.0f);
		glVertex2f(center.x, center.y);
		glEnd();
	}

	glFlush();
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'c':
		// 회전하지 않을 경우에 초기화
		if (!animation_mode) {
			p_click = p_size = 0;
			glutPostRedisplay();
		}
		break;
	case 'p':
		// 적어도 3개 이상 점이 있을 경우 다각형 형성 
		if (p_size < 3) {
			fprintf(stdout, "##### Error : 적어도 3개의 점을 선택해주세요!!! #####\n");
		}
		else {
			p_click = 1;
			glutPostRedisplay();
		}
		break;
	case 'r':
		// 다각형이 만들어진 경우(p_click = 1), 반시계 방향으로 0.5 초씩 30도씩 회전
		if (p_click) {
			animation_mode = 1 - animation_mode;
			if (animation_mode)
				glutTimerFunc(500, timer, 0);
		}
		break;
	case 'f':
		// 프로그램 종료
		glutLeaveMainLoop();
		break;
	}
}

void special(int key, int x, int y) {
	switch (key) {
	// 왼쪽으로 0.01 만큼 이동
	case GLUT_KEY_LEFT:
		if (p_click && !animation_mode) {
			for (int i = 0; i < p_size; i++)
				p[i].x -= 0.01f;
			glutPostRedisplay();
		}
		break;
	// 오른쪽으로 0.01 만큼 이동
	case GLUT_KEY_RIGHT:
		if (p_click && !animation_mode) {
			for (int i = 0; i < p_size; i++)
				p[i].x += 0.01f;
			glutPostRedisplay();
		}
		break;
	// 아랫쪽으로 0.01 만큼 이동
	case GLUT_KEY_DOWN:
		if (p_click && !animation_mode) {
			for (int i = 0; i < p_size; i++)
				p[i].y -= 0.01f;
			glutPostRedisplay();
		}
		break;
	// 윗쪽으로 0.01 만큼 이동
	case GLUT_KEY_UP:
		if (p_click && !animation_mode) {
			for (int i = 0; i < p_size; i++)
				p[i].y += 0.01f;
			glutPostRedisplay();
		}
		break;
	}
}

void mousepress(int button, int state, int x, int y) {
	// 왼쪽 마우스를 클릭한 상태에서 shift 키를 누른 경우
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN) 
		&& (glutGetModifiers() == GLUT_ACTIVE_SHIFT)) {
		// 다각형이 만들어지지 않았다면 화면에 점을 추가한다.
		if (!p_click) {
			p[p_size].x = (float)(x - (window_width / 2)) / (float)(window_width / 2);
			p[p_size].y = (float)(y - (window_height / 2)) / (float)(window_height / 2);
			p[p_size].y = -p[p_size].y;
			p_size++;
		}
	}
	// 오른쪽 버튼을 누른 경우 : rightbuttonpressed = 1
	// 오른쪽 버튼을 누르지 않은 경우 : rightbuttonpressed = 0
	if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN))
		rightbuttonpressed = 1;
	else if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_UP))
		rightbuttonpressed = 0;
}

void mousemove(int x, int y) {
	// 오른쪽 마우스를 누른 상태에서 다각형이 만들어진 경우와 회전하지 않을 경우
	// 현재 이동한 좌표 (x, y)와 이전 좌표 (prev_x, prev_y) 를 가지고 좌표를 이동시킨다.
	if (rightbuttonpressed && p_click && !animation_mode) {
		int del_x = x - prev_x, del_y = y - prev_y;
		for (int i = 0; i < p_size; i++) {
			p[i].x += (float)(del_x) / (float)(window_width);
			p[i].y -= (float)(del_y) / (float)(window_height);
		}
		glutPostRedisplay();
		prev_x = x, prev_y = y;
	}
}

void reshape(int width, int height) {
	// 윈도우 크기 변경 시 가로-세로 비율을 유지하면서 윈도우의 내용을 변경하도록 함
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
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 4
void main(int argc, char* argv[]) {
	char program_name[64] = "Sogang CSE4170 HW1_20192138_조명재";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'p', 'c', 'r', 'f'",
		"    - Special keys used: LEFT, RIGHT, UP, DOWN",
		"    - Mouse used: L-click, R-click and move",
		"    - Other operations: window size change"
	};

	glutInit(&argc, argv);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE); // <-- Be sure to use this profile for this example code!
 //	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutInitDisplayMode(GLUT_RGBA);

	glutInitWindowSize(500, 500);
	glutInitWindowPosition(500, 200);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	// glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_EXIT); // default
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// 프로그램 시작
	glutMainLoop();
	fprintf(stdout, "^^^ The control is at the end of main function now.\n\n");
}