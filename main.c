#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

#include <GL/glut.h>

#include "vector2.h"
#include "util.h"

#define DEFAULT_PAN_X		0.5f
#define DEFAULT_PAN_Y		0.5f
#define DEFAULT_SCALE		0.4f

static int width = 1024, height = 1024;
static enum { NONE, PANNING, SCALING } cur_op = NONE;
static int last_x, last_y;

static float pan_x = DEFAULT_PAN_X;
static float pan_y = DEFAULT_PAN_Y;
static float scale = DEFAULT_SCALE;

#define MAX_POINTS		64
#define POINTS_NAME		1000

static struct vector2 points[MAX_POINTS];
static int nr_points = 0;

static void bezier(struct vector2 *res, const struct vector2 *pts, int nr_pts, float t)
{
	int i, j;
	struct vector2 buf[nr_pts], buf2[nr_pts];
	struct vector2 *last = buf, *curr = buf2;

	memcpy(last, pts, sizeof(*last) * nr_pts);
	for (i = 0; i < nr_pts - 1; i++) {
		for (j = 0; j < nr_pts - i - 1; j++)
			vector2_lerp(&curr[j], &last[j], &last[j+1], t);
		SWAP(struct vector2 *, last, curr);
	}
	*res = last[0];
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
	int i;

	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(pan_x, pan_y, 0.0f);
	glScalef(scale, scale, 1.0f);

	glPointSize(10.0f);
	for (i = 0; i < nr_points; i++) {
		glColor3f(1.0f, 1.0f, 1.0f);
		glPushName(POINTS_NAME + i);
		glBegin(GL_POINTS);
		glVertex2f(points[i].x, points[i].y);
		glEnd();
		glPopName();
	}

#define MAX_SUBD		1500
	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	for (i = 0; i <= MAX_SUBD; i++) {
		struct vector2 pt;
		bezier(&pt, points, nr_points, (float) i / MAX_SUBD);
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

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
}

static int check_hits(int x, int y)
{
#define X_PICK_SIZE		20 /* FIXME: This should be the same as point size */
#define Y_PICK_SIZE		20
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
	gluPickMatrix(x, viewport[3] - y, X_PICK_SIZE, Y_PICK_SIZE, viewport);
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
		break;
	}
}

static void mouse(int button, int state, int x, int y)
{
	cur_op = NONE;
	if (glutGetModifiers() & GLUT_ACTIVE_ALT) {
		if (state == GLUT_DOWN) {
			if (button == GLUT_LEFT_BUTTON)
				cur_op = PANNING;
			else if (button == GLUT_RIGHT_BUTTON)
				cur_op = SCALING;
		}
	} else  if (state == GLUT_DOWN) {
		int active_point = check_hits(x, y);
		if (button == GLUT_LEFT_BUTTON) {
			/* Adding points */
			if (active_point == -1 && nr_points < MAX_POINTS) {
				GLdouble pt_x, pt_y, pt_z;
				GLdouble model[16];
				GLdouble proj[16];
				GLint viewport[4];

				glGetDoublev(GL_MODELVIEW_MATRIX, model);
				glGetDoublev(GL_PROJECTION_MATRIX, proj);
				glGetIntegerv(GL_VIEWPORT, viewport);
				gluUnProject(x, viewport[3] - y, 0.0, model, proj, viewport, &pt_x, &pt_y, &pt_z);

				points[nr_points++] = (struct vector2) { pt_x, pt_y };
			}
		} else if (button == GLUT_RIGHT_BUTTON) {
			/* Removing points */
			if (active_point != -1 && nr_points > 0) {
				memmove(points + active_point, points + active_point + 1,
					sizeof(*points) * (nr_points - active_point + 1));
				nr_points--;
			}
		}
	}

	switch (cur_op) {
	case PANNING:
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
