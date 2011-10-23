#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "vec2.h"
#include "curve.h"
#include "util.h"

#define DEFAULT_PAN_X		0.5f
#define DEFAULT_PAN_Y		0.5f
#define DEFAULT_SCALE		0.25f
#define DEFAULT_SLICES		(1 << 8)

static int width = 1024, height = 1024;
static enum { NONE, PANNING, SCALING, MOVING } cur_op = NONE;
static int last_x, last_y;
static float pan_x = DEFAULT_PAN_X;
static float pan_y = DEFAULT_PAN_Y;
static float scale = DEFAULT_SCALE;
static int slices = DEFAULT_SLICES;
static int selected_point = -1;
static int use_de_casteljau = 1;
static int render_control_polygon = 1;

#define POINT_SIZE		5
#define POINTS_NAME		1000
#define MAX_POINTS		64

static struct vec2 points[MAX_POINTS];
static int nr_points = 0;

static void unproject2(struct vec2 *res, float x, float y)
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

static void gl_printf(void *font, const char *format, ...)
{
	char str[512], *p;
	va_list args;

	va_start(args, format);
	vsprintf(str, format, args);
	va_end(args);

	for (p = str; *p; p++)
		glutBitmapCharacter(font, *p);
}

static void draw_frame(void)
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

static void display(void)
{
	static clock_t ticks;
	static int nr_frames, fps;
	clock_t now;
	int i;

	/* Calculate frame rate */
	nr_frames++;
	now = clock();
	if (now - ticks >= CLOCKS_PER_SEC) {
		fps = nr_frames;
		ticks = now;
		nr_frames = 0;
	}

	/* Start rendering */
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(pan_x, pan_y, 0.0f);
	glScalef(scale, scale, 1.0f);

	/* Render control polygon */
	if (render_control_polygon) {
		glBegin(GL_LINE_STRIP);
		glColor3f(0.0f, 0.0f, 1.0f);
		for (i = 0; i < nr_points; i++)
			glVertex2f(points[i].x, points[i].y);
		glEnd();
	}

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
		struct vec2 pt = { 0.0f, 0.0f };
		if (use_de_casteljau)
			bezier_casteljau(&pt, points, nr_points, (float) i / slices);
		else
			bezier_bernstein(&pt, points, nr_points, (float) i / slices);
		glVertex2f(pt.x, pt.y);
	}
	glEnd();

	draw_frame();

	/* Render fps counter */
	glPushMatrix();
	glLoadIdentity();
	glColor3f(1.0, 1.0f, 1.0f);
	glRasterPos2f(0.925f, 0.975f);
	gl_printf(GLUT_BITMAP_HELVETICA_18, "%d fps", fps);
	glPopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
}

static void reshape(int w, int h)
{
	width = w;
	height = h;

	/* Set nice antialiasing settings */
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
	gluPickMatrix(x, viewport[3] - y, POINT_SIZE * 1.5f, POINT_SIZE * 1.5f, viewport);
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
		for (j = 0; j < (int) nr_names; j++) {
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
		if (slices < (1 << 11))
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
	case 'p':
		render_control_polygon = !render_control_polygon;
		break;
	}
}

static void mouse(int button, int state, int x, int y)
{
	static const int cursor[] = {
		GLUT_CURSOR_RIGHT_ARROW,	/* NONE */
		GLUT_CURSOR_CROSSHAIR,		/* PANNING */
		GLUT_CURSOR_UP_DOWN,		/* SCALING */
		GLUT_CURSOR_CROSSHAIR		/* MOVING */
	};

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

	glutSetCursor(cursor[cur_op]);

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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(width, height);
	glutCreateWindow("bezier");

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();
	return 0;
}
