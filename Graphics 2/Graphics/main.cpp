
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"

const int MSZ = 100;
const int SPACE = 0;
const int WALL = 1;
const int START = 2;
const int TARGET = 3;




int maze[MSZ][MSZ] = { 0 };

void initMaze();

void init()
{
	srand(time(0));
	glClearColor(0.5,0.5,0.5,0);// color of window background
	glOrtho(0, MSZ, 0, MSZ, -1, 1); // set the coordinates system
	initMaze();
}

void initMaze()
{
	int i, j;

	for (i = 0;i < MSZ;i++)
	{
		maze[0][i] = WALL;
		maze[MSZ-1][i] = WALL;
		maze[i][0] = WALL;
		maze[i][MSZ-1] = WALL;
	}
	// setup walls and spaces
	for (i = 1;i < MSZ-1;i++)
	{
		for (j = 1;j < MSZ-1;j++)
		{
			if (i % 2 == 0) // walls
			{
				if (rand() % 100 < 70) // 70% walls
					maze[i][j] = WALL;
				else
					maze[i][j] = SPACE;
			}
			else // odd lines: SPACES
			{
				if (rand() % 100 < 85) // 85% spaces
					maze[i][j] = SPACE;
				else 					
					maze[i][j] = WALL;
			}
		}
	}

	maze[MSZ / 2][MSZ / 2] = START;
	maze[rand() % MSZ][rand() % MSZ] = TARGET;
}


void DrawMaze()
{
	int i, j;

	for(i=0;i<MSZ;i++)
		for (j = 0;j < MSZ;j++)
		{
			switch (maze[i][j])
			{
			case WALL:
				glColor3d(0.4, 0, 0); // dark red
				break;
			case SPACE:
				glColor3d(0.8, 0.8, 0.8); // lt. gray
				break;
			case START:
				glColor3d(0.6, 0.8, 1); // lt. blue
				break;
			case TARGET:
				glColor3d(1, 0,0); // red
				break;
			}
			glBegin(GL_POLYGON);
			glVertex2d(j, i);
			glVertex2d(j, i+1);
			glVertex2d(j+1, i+1);
			glVertex2d(j+1, i);
			glEnd();
		}
}

// all drawings are here
void display()
{
	glClear(GL_COLOR_BUFFER_BIT); // clean frame buffer

	DrawMaze();

	glutSwapBuffers(); // show all
}

// here are the changes
void idle() 
{
	

	glutPostRedisplay(); // call to refresh window
}


void main(int argc, char* argv[]) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("First Example");

	// callbacks
	glutDisplayFunc(display); // refresh window
	glutIdleFunc(idle); // background

	init();

	glutMainLoop();
}