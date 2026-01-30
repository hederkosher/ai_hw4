#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include "Node.h"
#include "CompareNodes.h"

#include <vector>
#include <queue>
#include <iostream>
#include <algorithm>

using namespace std;

const int MSZ = 100;
const int SPACE = 0;
const int WALL = 1;
const int START = 2;
const int TARGET = 3;
const int GRAY = 4;
const int BLACK = 5;
const int PATH = 6;

int maze[MSZ][MSZ] = { 0 };
int startRow, startCol, targetRow, targetCol;
vector<Node> grays;
vector<Node> blacks;
priority_queue<Node*, vector<Node*>, CompareNodes> pq;
Node* pCurrent = nullptr;
Node* pTarget = nullptr;
vector<Node*> path;

void initMaze();
void CheckNeighbor(Node* pNeighbor, int targetRow, int targetCol);
void AStar();
void DrawMaze();
void DrawPath();

void init()
{
	srand(time(0));
	glClearColor(0.5, 0.5, 0.5, 0); // color of window background
	glOrtho(0, MSZ, 0, MSZ, -1, 1); // set the coordinates system
	initMaze();
}

void initMaze()
{
	int i, j;

	for (i = 0; i < MSZ; i++)
	{
		maze[0][i] = WALL;
		maze[MSZ - 1][i] = WALL;
		maze[i][0] = WALL;
		maze[i][MSZ - 1] = WALL;
	}
	// setup walls and spaces
	for (i = 1; i < MSZ - 1; i++)
	{
		for (j = 1; j < MSZ - 1; j++)
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

	startRow = MSZ / 2;
	startCol = MSZ / 2;
	maze[startRow][startCol] = START;

	// Find a valid target position (must be SPACE)
	do {
		targetRow = rand() % MSZ;
		targetCol = rand() % MSZ;
	} while (maze[targetRow][targetCol] != SPACE || 
		(targetRow == startRow && targetCol == startCol));
	
	maze[targetRow][targetCol] = TARGET;
}

void CheckNeighbor(Node* pNeighbor, int targetRow, int targetCol)
{
	// pNeighbor can be:
	// black - do nothing
	// white - paint it gray and add it to pq
	// gray - update if better path found

	int row = pNeighbor->getRow();
	int col = pNeighbor->getCol();

	// Check if it's a wall or out of bounds
	if (row < 0 || row >= MSZ || col < 0 || col >= MSZ || maze[row][col] == WALL)
	{
		delete pNeighbor;
		return;
	}

	// Check if it's black (already processed)
	vector<Node>::iterator itBlack = find(blacks.begin(), blacks.end(), *pNeighbor);
	if (itBlack != blacks.end())
	{
		delete pNeighbor;
		return;
	}

	// Check if it's gray (already in queue)
	vector<Node>::iterator itGray = find(grays.begin(), grays.end(), *pNeighbor);
	if (itGray != grays.end())
	{
		// If we found a better path, update it
		if (pNeighbor->getF() < itGray->getF())
		{
			// Update the existing gray node with better path
			itGray->setParent(pNeighbor->getParent());
			itGray->setG(pNeighbor->getG());
			itGray->ComputeH(targetRow, targetCol);
			itGray->ComputeF();
			pq.push(&(*itGray));
		}
		delete pNeighbor;
		return;
	}

	// It's white - add it to grays and pq
	pNeighbor->ComputeH(targetRow, targetCol);
	pNeighbor->ComputeF();
	grays.push_back(*pNeighbor);
	pq.push(&grays.back());
	
	// Mark as gray in maze for visualization
	if (maze[row][col] == SPACE)
		maze[row][col] = GRAY;
}

void AStar()
{
	grays.clear();
	blacks.clear();
	while (!pq.empty())
		pq.pop();
	path.clear();
	if (pTarget != nullptr)
	{
		delete pTarget;
		pTarget = nullptr;
	}

	// Setup first Node (START position)
	Node startNode(startRow, startCol, nullptr, 0);
	startNode.ComputeH(targetRow, targetCol);
	startNode.ComputeF();
	grays.push_back(startNode);
	pq.push(&grays.back());

	while (!pq.empty())
	{
		// pick the BEST node from pq
		pCurrent = pq.top();
		
		// check for success!
		if (pCurrent->getRow() == targetRow && pCurrent->getCol() == targetCol)
		{
			cout << "The solution has been found!\n";
			// Reconstruct path by following parent pointers
			Node* p = pCurrent;
			while (p != nullptr)
			{
				path.push_back(p);
				Node* parent = p->getParent();
				if (parent != nullptr)
				{
					// Find parent in blacks
					vector<Node>::iterator it = find(blacks.begin(), blacks.end(), *parent);
					if (it != blacks.end())
						p = &(*it);
					else
					{
						// Find parent in grays
						it = find(grays.begin(), grays.end(), *parent);
						if (it != grays.end())
							p = &(*it);
						else
							p = nullptr;
					}
				}
				else
					p = nullptr;
			}
			reverse(path.begin(), path.end());
			return;
		}

		// extract it from pq
		pq.pop();
		
		// and paint it black
		blacks.push_back(*pCurrent);
		vector<Node>::iterator itGray = find(grays.begin(), grays.end(), *pCurrent);
		if (itGray == grays.end())
		{
			cout << "Error: pCurrent wasn't found in grays\n";
			return;
		}
		grays.erase(itGray);
		
		// Mark as black in maze for visualization
		int row = pCurrent->getRow();
		int col = pCurrent->getCol();
		if (maze[row][col] != START && maze[row][col] != TARGET)
			maze[row][col] = BLACK;

		// Store pointer to current node in blacks (for parent references)
		Node* pCurrentInBlacks = &blacks.back();
		
		// check the neighbors of pCurrent
		// try UP
		if (row > 0)
		{
			Node* pNeighbor = new Node(row - 1, col, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, targetRow, targetCol);
		}
		// try DOWN
		if (row < MSZ - 1)
		{
			Node* pNeighbor = new Node(row + 1, col, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, targetRow, targetCol);
		}
		// try LEFT
		if (col > 0)
		{
			Node* pNeighbor = new Node(row, col - 1, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, targetRow, targetCol);
		}
		// try RIGHT
		if (col < MSZ - 1)
		{
			Node* pNeighbor = new Node(row, col + 1, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, targetRow, targetCol);
		}
	}
	// if we arrive to this point there is no solution
	cout << "No solution found!\n";
}

void DrawMaze()
{
	int i, j;

	for (i = 0; i < MSZ; i++)
		for (j = 0; j < MSZ; j++)
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
				glColor3d(1, 0, 0); // red
				break;
			case GRAY:
				glColor3d(0.5, 0.5, 1); // light blue (explored)
				break;
			case BLACK:
				glColor3d(0.3, 0.3, 0.3); // dark gray (processed)
				break;
			case PATH:
				glColor3d(0, 1, 0); // green (path)
				break;
			}
			glBegin(GL_POLYGON);
			glVertex2d(j, i);
			glVertex2d(j, i + 1);
			glVertex2d(j + 1, i + 1);
			glVertex2d(j + 1, i);
			glEnd();
		}
}

void DrawPath()
{
	// Draw the path in green
	for (size_t i = 0; i < path.size(); i++)
	{
		int row = path[i]->getRow();
		int col = path[i]->getCol();
		if (maze[row][col] != START && maze[row][col] != TARGET)
		{
			glColor3d(0, 1, 0); // green
			glBegin(GL_POLYGON);
			glVertex2d(col, row);
			glVertex2d(col, row + 1);
			glVertex2d(col + 1, row + 1);
			glVertex2d(col + 1, row);
			glEnd();
		}
	}
}

// all drawings are here
void display()
{
	glClear(GL_COLOR_BUFFER_BIT); // clean frame buffer

	DrawMaze();
	DrawPath();

	glutSwapBuffers(); // show all
}

// here are the changes
void idle()
{
	glutPostRedisplay(); // call to refresh window
}

void mouseClick(int button, int state, int x, int y)
{
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		// Reset maze colors (except START, TARGET, WALL)
		for (int i = 0; i < MSZ; i++)
			for (int j = 0; j < MSZ; j++)
				if (maze[i][j] == GRAY || maze[i][j] == BLACK || maze[i][j] == PATH)
					maze[i][j] = SPACE;
		
		// Restore START and TARGET
		maze[startRow][startCol] = START;
		maze[targetRow][targetCol] = TARGET;
		
		// Run A* algorithm
		AStar();
	}
}

void main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("Maze A* Pathfinding");

	// callbacks
	glutDisplayFunc(display); // refresh window
	glutIdleFunc(idle); // background
	glutMouseFunc(mouseClick);

	init();

	glutMainLoop();
}
