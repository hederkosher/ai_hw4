#include "Node.h"
#include "glut.h"
#include <math.h>

Node::Node()
{
	row = 0;
	col = 0;
	parent = nullptr;
	g = 0;
	h = 0;
	f = 0;
}

Node::Node(int r, int c, Node* p, double g_val)
{
	row = r;
	col = c;
	parent = p;
	g = g_val;
	h = 0;
	f = 0;
}

void Node::Draw()
{
	glColor3d(0, 1, 0); // green for path nodes
	glBegin(GL_POLYGON);
	glVertex2d(col, row);
	glVertex2d(col, row + 1);
	glVertex2d(col + 1, row + 1);
	glVertex2d(col + 1, row);
	glEnd();
}

void Node::ComputeH(int targetRow, int targetCol)
{
	// Manhattan distance heuristic
	h = abs(row - targetRow) + abs(col - targetCol);
}

void Node::ComputeF()
{
	f = g + h;
}

bool Node::operator==(const Node &other)
{
	return (row == other.row && col == other.col);
}

