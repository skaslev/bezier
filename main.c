#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include "vector2.h"
#include "curve.h"
#include "util.h"

#define DEFAULT_PAN_X		0.5f
#define DEFAULT_PAN_Y		0.5f
#define DEFAULT_SCALE		0.25f
#define DEFAULT_SLICES		(1 << 10)

static int width = 1024, height = 1024;
static enum { NONE, PANNING, SCALING, MOVING } cur_op = NONE;
static int last_x, last_y;
static float pan_x = DEFAULT_PAN_X;
static float pan_y = DEFAULT_PAN_Y;
static float scale = DEFAULT_SCALE;
static int slices = DEFAULT_SLICES;
static int selected_point = -1;
static int use_de_casteljau = 1;

#define POINT_SIZE		10
#define POINTS_NAME		1000
#define MAX_POINTS		64

static struct vector2 points[MAX_POINTS];
static int nr_points = 0;

static void unproject2(struct vector2 *res, float x, float y)
{
	GLdouble pt_x, pt_y, pt_z;
	GLdouble model[16];
	GLdouble proj[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluUnProject(x, viewport[3] - y, 0.0, model, proj, viewport, &pt_x, &pt_y, &pt_z);

	res->x = pt_x;
	res->y = pt_y;
}

static void frame()
{
	/* Draw the x axis */
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glEnd();

	/* Draw the y axis */
	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glEnd();
}

static void display()
{
	static clock_t ticks;
	static int nframes;
	clock_t now;
	int i;

	/* Calculate frame rate */
	nframes++;
	now = clock();
	if (now - ticks >= CLOCKS_PER_SEC) {
		printf("\r%dfps", nframes);
		fflush(stdout);
		ticks = now;
		nframes = 0;
	}

	/* Start rendering */
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(pan_x, pan_y, 0.0f);
	glScalef(scale, scale, 1.0f);

	/* Render control points */
	glPointSize(POINT_SIZE);
	for (i = 0; i < nr_points; i++) {
		glColor3f(1.0f, 1.0f, 1.0f);
		glPushName(POINTS_NAME + i);
		glBegin(GL_POINTS);
		glVertex2f(points[i].x, points[i].y);
		glEnd();
		glPopName();
	}

	/* Render the curve itself */
	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	for (i = 0; i <= slices; i++) {
		struct vector2 pt = {};
		if (use_de_casteljau)
			bezier(&pt, points, nr_points, (float) i / slices);
		else
			bezier_bernstein(&pt, points, nr_points, (float) i / slices);
		glVertex2f(pt.x, pt.y);
	}
	glEnd();

	frame();

	glutSwapBuffers();
	glutPostRedisplay();
}

static void reshape(int w, int h)
{
	width = w;
	height = h;

	/* Set nice antialiasing settings */
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	/* Set viewport and projection matrices */
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
}

static int check_hits(int x, int y)
{
	GLuint buffer[256], *ptr;
	GLint hits;
	GLint viewport[4];
	GLdouble proj[16];
	int i, j;

	glSelectBuffer(ARRAY_SIZE(buffer), buffer);
	glRenderMode(GL_SELECT);
	glInitNames();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	/* Set proper projection matrix */
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glLoadIdentity();
	gluPickMatrix(x, viewport[3] - y, POINT_SIZE, POINT_SIZE, viewport);
	glMultMatrixd(proj);

	display();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	hits = glRenderMode(GL_RENDER);

	/* Process hits */
	ptr = buffer;
	for (i = 0; i < hits; i++) {
		GLuint nr_names = *ptr++;
		ptr++;	/* zmin */
		ptr++;	/* zmax */
		for (j = 0; j < nr_names; j++) {
			GLuint name = *ptr++;
			int idx = name - POINTS_NAME;
			if (idx >= 0 && idx < nr_points)
				return idx;
		}
	}
	return -1;
}

static void keyboard(unsigned char key, int x, int y)
{
	enum { ESC = 27, CTRL_Q = 17 };
	switch (key) {
	case ESC: case CTRL_Q:
		exit(0);
		break;
	case 'r':
		pan_x = DEFAULT_PAN_X;
		pan_y = DEFAULT_PAN_Y;
		scale = DEFAULT_SCALE;
		slices = DEFAULT_SLICES;
		break;
	case '-': case '_':
		if (slices != 1)
			slices >>= 1;
		printf("Rendering %d slices\n", slices);
		break;
	case '=': case '+':
		if (slices < (1 << 15))
			slices <<= 1;
		printf("Rendering %d slices\n", slices);
		break;
	case 'c':
		use_de_casteljau = !use_de_casteljau;
		if (use_de_casteljau)
			printf("Switched to de Casteljau's algorithm\n");
		else
			printf("Switched to Bernstein polynomials\n");
		break;
	}
}

static void mouse(int button, int state, int x, int y)
{
	cur_op = NONE;
	if ((glutGetModifiers() & GLUT_ACTIVE_ALT) && state == GLUT_DOWN) {
		if (button == GLUT_LEFT_BUTTON)
			cur_op = PANNING;
		else if (button == GLUT_RIGHT_BUTTON)
			cur_op = SCALING;
	} else  if (state == GLUT_DOWN) {
		int active_point = check_hits(x, y);
		if (button == GLUT_LEFT_BUTTON && active_point != -1) {
			/* Moving point */
			selected_point = active_point;
			cur_op = MOVING;
		} else if (button == GLUT_MIDDLE_BUTTON && active_point == -1 && nr_points < MAX_POINTS) {
			/* Add point */
			unproject2(&points[nr_points++], x, y);
		} else if (button == GLUT_RIGHT_BUTTON && active_point != -1 && nr_points > 0) {
			/* Remove point */
			memmove(points + active_point, points + active_point + 1,
				sizeof(*points) * (nr_points - active_point + 1));
			nr_points--;
		}
	}

	switch (cur_op) {
	case PANNING: case MOVING:
		glutSetCursor(GLUT_CURSOR_CROSSHAIR);
		break;
	case SCALING:
		glutSetCursor(GLUT_CURSOR_UP_DOWN);
		break;
	case NONE:
	default:
		glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
		break;
	}

	last_x = x;
	last_y = y;
}

static void motion(int x, int y)
{
	float dx = (float) (x - last_x) / width;
	float dy = (float) (y - last_y) / height;

	if (cur_op == PANNING) {
		pan_x += dx;
		pan_y -= dy;
	} else if (cur_op == SCALING) {
		scale += dy;
	} else if (cur_op == MOVING) {
		unproject2(&points[selected_point], x, y);
	}

	last_x = x;
	last_y = y;
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize(width, height);
	glutCreateWindow("CAGD");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();
	return 0;
}
