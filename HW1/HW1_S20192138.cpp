#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

// HW1 : GLUT ��Ŷ�� ����� ������ �ٰ��� ���� ��� ����
// 20192138 ������ 

typedef struct point {
	float x;
	float y;
} Point;

int window_width = 0, window_height = 0; // To be used later.
int prev_x = 0, prev_y = 0; // ���� ��ǥ�� ���� ����
int rightbuttonpressed = 0; // ������ ��ư�� Ŭ���ߴ����� ���� ����
int animation_mode = 0; // 30���� ȸ���ϴ� ��� 1, �ƴ� ��� 0
unsigned int timestamp = 0; // timer �Լ����� �ʿ��� ����
float r = 0.0f, g = 1.0f, b = 0.0f; // ������ �ʷϻ�����
Point center; // �ٰ����� �����߽ɿ� ���� ����
Point p[128];
int p_size = 0; // ������ ����
int p_click = 0; // �ٰ����� ����������� Ȯ���ϴ� �뵵 

void timer(int value) {
	float next_x, next_y;
	// ȸ�� ������ 30���� ���Ͽ���.
	// cos(30) = 0.866 (�뷫������), sin(30) = 0.5 �̴�.
	// ȸ����ȯ�� �̿��Ͽ� ������ ��ġ�� �����Ͽ���.
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

	// ȭ�鿡 Ŭ���� �� �׸��� (�Ķ���)
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glColor3f(0.0f, 0.0f, 1.0f); // ���� ��� �Ķ������� ����
	center.x = center.y = 0.0f;
	for (int i = 0; i < p_size; i++) {
		glVertex2f(p[i].x, p[i].y);
		center.x += p[i].x;
		center.y += p[i].y;
	}
	// Null Pointer Exception �� �����ؾ� �Ѵ�.
	// ���� ���� ��� �� �� ������ ��� �ٰ����� �����߽��� ���Ѵ�.
	if (p_size) { 
		center.x /= p_size;
		center.y /= p_size;
	}
	glEnd();

	// ȭ�鿡 Ŭ���� ������ ������ �̾��ֱ� (������)
	glLineWidth(2.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	for (int i = 0; i < p_size - 1; i++) {
		glVertex2f(p[i].x, p[i].y); 
		glVertex2f(p[i + 1].x, p[i + 1].y);
	}
	// 'p' Ű�� �Է��ϸ�, �ٰ����� �׷������� ù ���� ���� ������ ���� ����
	if (p_click) {
		glVertex2f(p[0].x, p[0].y);
		glVertex2f(p[p_size - 1].x, p[p_size - 1].y);
	}
	glEnd();

	// ���� �߽� (�����)
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
		// ȸ������ ���� ��쿡 �ʱ�ȭ
		if (!animation_mode) {
			p_click = p_size = 0;
			glutPostRedisplay();
		}
		break;
	case 'p':
		// ��� 3�� �̻� ���� ���� ��� �ٰ��� ���� 
		if (p_size < 3) {
			fprintf(stdout, "##### Error : ��� 3���� ���� �������ּ���!!! #####\n");
		}
		else {
			p_click = 1;
			glutPostRedisplay();
		}
		break;
	case 'r':
		// �ٰ����� ������� ���(p_click = 1), �ݽð� �������� 0.5 �ʾ� 30���� ȸ��
		if (p_click) {
			animation_mode = 1 - animation_mode;
			if (animation_mode)
				glutTimerFunc(500, timer, 0);
		}
		break;
	case 'f':
		// ���α׷� ����
		glutLeaveMainLoop();
		break;
	}
}

void special(int key, int x, int y) {
	switch (key) {
	// �������� 0.01 ��ŭ �̵�
	case GLUT_KEY_LEFT:
		if (p_click && !animation_mode) {
			for (int i = 0; i < p_size; i++)
				p[i].x -= 0.01f;
			glutPostRedisplay();
		}
		break;
	// ���������� 0.01 ��ŭ �̵�
	case GLUT_KEY_RIGHT:
		if (p_click && !animation_mode) {
			for (int i = 0; i < p_size; i++)
				p[i].x += 0.01f;
			glutPostRedisplay();
		}
		break;
	// �Ʒ������� 0.01 ��ŭ �̵�
	case GLUT_KEY_DOWN:
		if (p_click && !animation_mode) {
			for (int i = 0; i < p_size; i++)
				p[i].y -= 0.01f;
			glutPostRedisplay();
		}
		break;
	// �������� 0.01 ��ŭ �̵�
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
	// ���� ���콺�� Ŭ���� ���¿��� shift Ű�� ���� ���
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN) 
		&& (glutGetModifiers() == GLUT_ACTIVE_SHIFT)) {
		// �ٰ����� ��������� �ʾҴٸ� ȭ�鿡 ���� �߰��Ѵ�.
		if (!p_click) {
			p[p_size].x = (float)(x - (window_width / 2)) / (float)(window_width / 2);
			p[p_size].y = (float)(y - (window_height / 2)) / (float)(window_height / 2);
			p[p_size].y = -p[p_size].y;
			p_size++;
		}
	}
	// ������ ��ư�� ���� ��� : rightbuttonpressed = 1
	// ������ ��ư�� ������ ���� ��� : rightbuttonpressed = 0
	if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN))
		rightbuttonpressed = 1;
	else if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_UP))
		rightbuttonpressed = 0;
}

void mousemove(int x, int y) {
	// ������ ���콺�� ���� ���¿��� �ٰ����� ������� ���� ȸ������ ���� ���
	// ���� �̵��� ��ǥ (x, y)�� ���� ��ǥ (prev_x, prev_y) �� ������ ��ǥ�� �̵���Ų��.
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
	// ������ ũ�� ���� �� ����-���� ������ �����ϸ鼭 �������� ������ �����ϵ��� ��
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
	char program_name[64] = "Sogang CSE4170 HW1_20192138_������";
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

	// ���α׷� ����
	glutMainLoop();
	fprintf(stdout, "^^^ The control is at the end of main function now.\n\n");
}