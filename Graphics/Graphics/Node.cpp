#include "Node.h"
#include "glut.h"
#include <math.h>

Node::Node()
{
	int i, j;

	for (i = 0;i < BSZ;i++)
		for (j = 0;j < BSZ;j++)
			board[i][j] = 1 + i * BSZ + j;

	board[BSZ - 1][BSZ - 1] = 0;
	emptyCol = BSZ - 1;
	emptyRow = BSZ - 1;
	parent = nullptr;

	g = 0;
	ComputeH();
	ComputeF();
}

Node::Node(Node* pn, int direction)
{
	int i, j;
	// copy the board
	for (i = 0;i < BSZ;i++)
		for (j = 0;j < BSZ;j++)
			board[i][j] = pn->board[i][j];
	emptyRow = pn->emptyRow;
	emptyCol = pn->emptyCol;
	parent = pn;

	g = pn->g + 1;

	switch (direction)
	{
	case UP:
		ExchangeCells(emptyRow - 1, emptyCol);
		break;
	case DOWN:
		ExchangeCells(emptyRow + 1, emptyCol);
		break;
	case LEFT:
		ExchangeCells(emptyRow , emptyCol- 1);
		break;
	case RIGHT:
		ExchangeCells(emptyRow, emptyCol + 1);
		break;
	}

	// update h and f
	ComputeH();
	ComputeF();
}

void Node::Draw()
{
	int i, j;
	for (i = 0;i < BSZ;i++)
		for (j = 0;j < BSZ;j++)
		{
			glColor3d(0.8, 0.8, 0.8);
			glBegin(GL_POLYGON);
			glVertex2d(j, i);
			glVertex2d(j + 1, i);
			glVertex2d(j + 1, i + 1);
			glVertex2d(j , i + 1);
			glEnd();
			glColor3d(0,0,0);

			glBegin(GL_LINE_LOOP);
			glVertex2d(j, i);
			glVertex2d(j + 1, i);
			glVertex2d(j + 1, i + 1);
			glVertex2d(j, i + 1);
			glEnd();

			// draw number
			if (board[i][j] != 0)
			{
				glRasterPos2d(j + 0.4, i + 0.6);
				if (board[i][j] >= 10) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, '1');
				glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, '0' + board[i][j] % 10);
			}
		}

}

void Node::ComputeH()
{
	int i, j;
	double sum = 0;

	for (i = 0;i < BSZ;i++)
		for (j = 0;j < BSZ;j++)
			sum += ManhattanDistance(i, j);


	h = sum;
}

void Node::ComputeF()
{
	f = g + h;
}

int Node::ManhattanDistance(int row, int col)
{
	int targetRow, targetCol;

	if (board[row][col] == 0)
		return (BSZ - 1) - row + (BSZ - 1) - col;
	else
	{
		targetRow = (board[row][col] - 1) / BSZ;
		targetCol = (board[row][col] - 1) % BSZ;
		return (abs(row - targetRow) + abs(col - targetCol));
	}
}

// exchanges cells if row/col is adjacent to empty cell
void Node::ExchangeCells(int row, int col)
{
	int horizontal_dist = abs(row - emptyRow);
	int vertical_dist = abs(col - emptyCol);

	if (horizontal_dist + vertical_dist == 1)
	{
		board[emptyRow][emptyCol] = board[row][col];
		board[row][col] = 0;
		emptyRow = row;
		emptyCol = col;
	}
}

// we shall need operator == for using find
bool Node::operator==(const Node &other)
{
	int i, j;

	for (i = 0;i < BSZ;i++)
		for (j = 0;j < BSZ;j++)
			if(other.board[i][j]!=board[i][j]) 	return false;

	return true;
}
