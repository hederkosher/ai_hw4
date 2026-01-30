
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"
#include "Node.h"
#include "CompareNodes.h"

#include <vector>
#include <queue>
#include<iostream>

using namespace std;



const int W = 600;
const int H = 600;

Node* pn=nullptr;

void init()
{
	glClearColor(0.5,0.5,0.5,0);// color of window background
	glOrtho(0,BSZ, BSZ, 0, -1, 1); // set the coordinates system

	pn = new Node();
}

void CheckNeighbor(Node* pNeighbor, vector<Node> &grays, 
	vector<Node> &blacks, priority_queue<Node*, vector<Node*>, CompareNodes> &pq)
{
	// pNeighbor can be:
	// black  -  do nothing
	// white -   paint it gray and add it to pq
	// target or gray can be simple if it is not better then the previous gray
	// can be tough if there is a need to update
}


void AStar()
{
	vector<Node> grays;
	vector<Node> blacks;
	priority_queue<Node*, vector<Node*>, CompareNodes> pq;
	Node* pCurrent;	
	Node* pNeighbor;
	
	vector<Node>::iterator itGray;

	// setup first Node
	grays.push_back(*pn);
	pq.push(pn);

	while (!pq.empty())
	{
		// pick the BEST node from pq
		pCurrent = pq.top();
		// check for success!
		if (pCurrent->getH() < 0.1) // the board is solved
		{
			cout << "The solution has been found!\n";
			return;
		}

		// extract it from pq
		pq.pop();		
		// and paint it black
		blacks.push_back(*pCurrent);
		itGray = find(grays.begin(), grays.end(), *pCurrent); // must implement operator ==
		if (itGray == grays.end())
		{
			cout << "Error: pCurrent wasn't found in grays\n";
			exit(1);
		}
		grays.erase(itGray);
		// check the neighbors of pCurrent
		// try UP
		if (pCurrent->getEmptyRow() > 0) // we can move up
		{
			pNeighbor = new Node(pCurrent, UP);
			CheckNeighbor(pNeighbor, grays, blacks, pq);
		}

	}
	// in our cas this is impossible
	// but if we arrive to this point there must be an internal error
	cout << "PQ is empty: shouldn't be in our case\n";

}



// all drawings are here
void display()
{
	glClear(GL_COLOR_BUFFER_BIT); // clean frame buffer


	if (pn != nullptr)
		pn->Draw();

	glutSwapBuffers(); // show all
}

// here are the changes
void idle() 
{
	

	glutPostRedisplay(); // call to refresh window
}


// x and y are the coordinates of click in pixels
void mouseClick(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		int row, col;
		col = BSZ * (x / (double)W);
		row = BSZ * (y / (double)H);
		pn->ExchangeCells(row, col);
	}
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		pn->ComputeH();
		pn->ComputeF();
		AStar();
	}

}

void main(int argc, char* argv[]) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(W, H);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("Puzzle");

	// callbacks
	glutDisplayFunc(display); // refresh window
	glutIdleFunc(idle); // background
	glutMouseFunc(mouseClick);

	init();

	glutMainLoop();
}