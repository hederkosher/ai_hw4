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
const int COIN = 2;
const int PACMAN = 3;
const int GHOST = 4;

const int MAX_BFS_DEPTH = 5; // Limited depth for BFS

int maze[MSZ][MSZ] = { 0 };
int pacmanRow, pacmanCol;
int ghostRows[3], ghostCols[3];
int coinsCollected = 0;
int totalCoins = 0;
bool gameRunning = true;

// Directions
const int UP = 1;
const int DOWN = 2;
const int LEFT = 3;
const int RIGHT = 4;

void initMaze();
void CheckNeighbor(Node* pNeighbor, vector<Node> &grays, 
	vector<Node> &blacks, priority_queue<Node*, vector<Node*>, CompareNodes> &pq, int targetRow, int targetCol);
vector<Node*> AStar(int startRow, int startCol, int targetRow, int targetCol);
int LimitedBFS(int startRow, int startCol, int maxDepth);
int FindNearestCoin(int row, int col);
int GetDirectionAwayFromGhost(int pacmanRow, int pacmanCol, int ghostRow, int ghostCol);
int GetDirectionTowardCoin(int pacmanRow, int pacmanCol, int coinRow, int coinCol);
void MovePacman();
void MoveGhosts();
void DrawMaze();
void DrawCharacters();

void init()
{
	srand(time(0));
	glClearColor(0.0, 0.0, 0.0, 0); // black background
	glOrtho(0, MSZ, 0, MSZ, -1, 1);
	initMaze();
}

void initMaze()
{
	int i, j;

	// Create walls around borders
	for (i = 0; i < MSZ; i++)
	{
		maze[0][i] = WALL;
		maze[MSZ - 1][i] = WALL;
		maze[i][0] = WALL;
		maze[i][MSZ - 1] = WALL;
	}

	// Setup walls and spaces
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

	// Place Pacman
	pacmanRow = MSZ / 2;
	pacmanCol = MSZ / 2;
	maze[pacmanRow][pacmanCol] = PACMAN;

	// Place 3 ghosts
	for (int g = 0; g < 3; g++)
	{
		do {
			ghostRows[g] = rand() % MSZ;
			ghostCols[g] = rand() % MSZ;
		} while (maze[ghostRows[g]][ghostCols[g]] != SPACE || 
			(ghostRows[g] == pacmanRow && ghostCols[g] == pacmanCol));
		maze[ghostRows[g]][ghostCols[g]] = GHOST;
	}

	// Place coins
	totalCoins = 0;
	for (i = 1; i < MSZ - 1; i++)
	{
		for (j = 1; j < MSZ - 1; j++)
		{
			if (maze[i][j] == SPACE && rand() % 100 < 20) // 20% chance for coin
			{
				maze[i][j] = COIN;
				totalCoins++;
			}
		}
	}
}

void CheckNeighbor(Node* pNeighbor, vector<Node> &grays, 
	vector<Node> &blacks, priority_queue<Node*, vector<Node*>, CompareNodes> &pq, int targetRow, int targetCol)
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
}

// A* algorithm to find path from start to target
vector<Node*> AStar(int startRow, int startCol, int targetRow, int targetCol)
{
	vector<Node> grays;
	vector<Node> blacks;
	priority_queue<Node*, vector<Node*>, CompareNodes> pq;
	Node* pCurrent;
	Node* pNeighbor;
	vector<Node*> path;

	// Setup first Node
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
			// Reconstruct path
			Node* p = pCurrent;
			while (p != nullptr)
			{
				path.push_back(p);
				Node* parent = p->getParent();
				if (parent != nullptr)
				{
					vector<Node>::iterator it = find(blacks.begin(), blacks.end(), *parent);
					if (it != blacks.end())
						p = &(*it);
					else
					{
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
			return path;
		}

		// extract it from pq
		pq.pop();

		// and paint it black
		blacks.push_back(*pCurrent);
		vector<Node>::iterator itGray = find(grays.begin(), grays.end(), *pCurrent);
		if (itGray == grays.end())
		{
			return path; // No path found
		}
		grays.erase(itGray);

		// Store pointer to current node in blacks
		Node* pCurrentInBlacks = &blacks.back();

		// check the neighbors of pCurrent
		int row = pCurrent->getRow();
		int col = pCurrent->getCol();

		// try UP
		if (row > 0)
		{
			pNeighbor = new Node(row - 1, col, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, grays, blacks, pq, targetRow, targetCol);
		}
		// try DOWN
		if (row < MSZ - 1)
		{
			pNeighbor = new Node(row + 1, col, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, grays, blacks, pq, targetRow, targetCol);
		}
		// try LEFT
		if (col > 0)
		{
			pNeighbor = new Node(row, col - 1, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, grays, blacks, pq, targetRow, targetCol);
		}
		// try RIGHT
		if (col < MSZ - 1)
		{
			pNeighbor = new Node(row, col + 1, pCurrentInBlacks, pCurrent->getG() + 1);
			CheckNeighbor(pNeighbor, grays, blacks, pq, targetRow, targetCol);
		}
	}
	return path; // No path found
}

// Limited-depth BFS to find nearest ghost
int LimitedBFS(int startRow, int startCol, int maxDepth)
{
	queue<pair<pair<int, int>, int>> q; // ((row, col), depth)
	vector<vector<bool>> visited(MSZ, vector<bool>(MSZ, false));

	q.push({{startRow, startCol}, 0});
	visited[startRow][startCol] = true;

	while (!q.empty())
	{
		int row = q.front().first.first;
		int col = q.front().first.second;
		int depth = q.front().second;
		q.pop();

		if (depth >= maxDepth)
			continue;

		// Check if this is a ghost position
		for (int g = 0; g < 3; g++)
		{
			if (row == ghostRows[g] && col == ghostCols[g])
			{
				return g; // Return ghost index
			}
		}

		// Check neighbors
		int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
		for (int d = 0; d < 4; d++)
		{
			int newRow = row + directions[d][0];
			int newCol = col + directions[d][1];

			if (newRow >= 0 && newRow < MSZ && newCol >= 0 && newCol < MSZ &&
				!visited[newRow][newCol] && maze[newRow][newCol] != WALL)
			{
				visited[newRow][newCol] = true;
				q.push({{newRow, newCol}, depth + 1});
			}
		}
	}
	return -1; // No ghost found within depth limit
}

int FindNearestCoin(int row, int col)
{
	queue<pair<int, int>> q;
	vector<vector<bool>> visited(MSZ, vector<bool>(MSZ, false));

	q.push({row, col});
	visited[row][col] = true;

	while (!q.empty())
	{
		int r = q.front().first;
		int c = q.front().second;
		q.pop();

		if (maze[r][c] == COIN)
		{
			return r * MSZ + c; // Return encoded position
		}

		// Check neighbors
		int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
		for (int d = 0; d < 4; d++)
		{
			int newRow = r + directions[d][0];
			int newCol = c + directions[d][1];

			if (newRow >= 0 && newRow < MSZ && newCol >= 0 && newCol < MSZ &&
				!visited[newRow][newCol] && maze[newRow][newCol] != WALL)
			{
				visited[newRow][newCol] = true;
				q.push({newRow, newCol});
			}
		}
	}
	return -1; // No coin found
}

int GetDirectionAwayFromGhost(int pacmanRow, int pacmanCol, int ghostRow, int ghostCol)
{
	int bestDir = UP;
	double maxDist = 0;

	int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
	int dirNames[4] = {UP, DOWN, LEFT, RIGHT};

	for (int d = 0; d < 4; d++)
	{
		int newRow = pacmanRow + directions[d][0];
		int newCol = pacmanCol + directions[d][1];

		if (newRow >= 0 && newRow < MSZ && newCol >= 0 && newCol < MSZ &&
			maze[newRow][newCol] != WALL)
		{
			double dist = abs(newRow - ghostRow) + abs(newCol - ghostCol);
			if (dist > maxDist)
			{
				maxDist = dist;
				bestDir = dirNames[d];
			}
		}
	}
	return bestDir;
}

int GetDirectionTowardCoin(int pacmanRow, int pacmanCol, int coinRow, int coinCol)
{
	int bestDir = UP;
	double minDist = MSZ * MSZ;

	int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
	int dirNames[4] = {UP, DOWN, LEFT, RIGHT};

	for (int d = 0; d < 4; d++)
	{
		int newRow = pacmanRow + directions[d][0];
		int newCol = pacmanCol + directions[d][1];

		if (newRow >= 0 && newRow < MSZ && newCol >= 0 && newCol < MSZ &&
			maze[newRow][newCol] != WALL)
		{
			double dist = abs(newRow - coinRow) + abs(newCol - coinCol);
			if (dist < minDist)
			{
				minDist = dist;
				bestDir = dirNames[d];
			}
		}
	}
	return bestDir;
}

void MovePacman()
{
	// Use limited-depth BFS to check for ghosts
	int nearestGhostIdx = LimitedBFS(pacmanRow, pacmanCol, MAX_BFS_DEPTH);

	int newRow = pacmanRow;
	int newCol = pacmanCol;

	if (nearestGhostIdx >= 0)
	{
		// Ghost found - move away from nearest ghost
		int dir = GetDirectionAwayFromGhost(pacmanRow, pacmanCol, 
			ghostRows[nearestGhostIdx], ghostCols[nearestGhostIdx]);
		switch (dir)
		{
		case UP: newRow--; break;
		case DOWN: newRow++; break;
		case LEFT: newCol--; break;
		case RIGHT: newCol++; break;
		}
	}
	else
	{
		// No ghost found - move toward nearest coin
		int coinPos = FindNearestCoin(pacmanRow, pacmanCol);
		if (coinPos >= 0)
		{
			int coinRow = coinPos / MSZ;
			int coinCol = coinPos % MSZ;
			int dir = GetDirectionTowardCoin(pacmanRow, pacmanCol, coinRow, coinCol);
			switch (dir)
			{
			case UP: newRow--; break;
			case DOWN: newRow++; break;
			case LEFT: newCol--; break;
			case RIGHT: newCol++; break;
			}
		}
	}

	// Move Pacman
	if (newRow >= 0 && newRow < MSZ && newCol >= 0 && newCol < MSZ &&
		maze[newRow][newCol] != WALL)
	{
		// Collect coin if present
		if (maze[newRow][newCol] == COIN)
		{
			coinsCollected++;
		}

		maze[pacmanRow][pacmanCol] = SPACE;
		pacmanRow = newRow;
		pacmanCol = newCol;
		maze[pacmanRow][pacmanCol] = PACMAN;
	}
}

void MoveGhosts()
{
	for (int g = 0; g < 3; g++)
	{
		// Use A* to find path to Pacman
		vector<Node*> path = AStar(ghostRows[g], ghostCols[g], pacmanRow, pacmanCol);

		if (path.size() > 1)
		{
			// Move to next step in path
			Node* next = path[1];
			int newRow = next->getRow();
			int newCol = next->getCol();

			if (newRow >= 0 && newRow < MSZ && newCol >= 0 && newCol < MSZ &&
				maze[newRow][newCol] != WALL)
			{
				maze[ghostRows[g]][ghostCols[g]] = SPACE;
				ghostRows[g] = newRow;
				ghostCols[g] = newCol;
				maze[ghostRows[g]][ghostCols[g]] = GHOST;
			}
		}
	}
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
				glColor3d(0.2, 0.2, 0.5); // dark blue
				break;
			case SPACE:
				glColor3d(0.0, 0.0, 0.0); // black
				break;
			case COIN:
				glColor3d(1.0, 0.84, 0.0); // gold
				break;
			case PACMAN:
				glColor3d(1.0, 1.0, 0.0); // yellow
				break;
			case GHOST:
				glColor3d(1.0, 0.0, 0.0); // red
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

void DrawCharacters()
{
	// Draw Pacman (circle)
	glColor3d(1.0, 1.0, 0.0); // yellow
	glBegin(GL_TRIANGLE_FAN);
	double centerX = pacmanCol + 0.5;
	double centerY = pacmanRow + 0.5;
	glVertex2d(centerX, centerY);
	for (int i = 0; i <= 20; i++)
	{
		double angle = 2.0 * 3.14159 * i / 20.0;
		glVertex2d(centerX + 0.4 * cos(angle), centerY + 0.4 * sin(angle));
	}
	glEnd();

	// Draw Ghosts (circles)
	for (int g = 0; g < 3; g++)
	{
		glColor3d(1.0, 0.0, 0.0); // red
		glBegin(GL_TRIANGLE_FAN);
		double centerX = ghostCols[g] + 0.5;
		double centerY = ghostRows[g] + 0.5;
		glVertex2d(centerX, centerY);
		for (int i = 0; i <= 20; i++)
		{
			double angle = 2.0 * 3.14159 * i / 20.0;
			glVertex2d(centerX + 0.4 * cos(angle), centerY + 0.4 * sin(angle));
		}
		glEnd();
	}

	// Draw Coins (small circles)
	for (int i = 0; i < MSZ; i++)
	{
		for (int j = 0; j < MSZ; j++)
		{
			if (maze[i][j] == COIN)
			{
				glColor3d(1.0, 0.84, 0.0); // gold
				glBegin(GL_TRIANGLE_FAN);
				double centerX = j + 0.5;
				double centerY = i + 0.5;
				glVertex2d(centerX, centerY);
				for (int k = 0; k <= 10; k++)
				{
					double angle = 2.0 * 3.14159 * k / 10.0;
					glVertex2d(centerX + 0.2 * cos(angle), centerY + 0.2 * sin(angle));
				}
				glEnd();
			}
		}
	}
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	DrawMaze();
	DrawCharacters();

	glutSwapBuffers();
}

void idle()
{
	if (gameRunning)
	{
		// Check if game over (Pacman caught or all coins collected)
		bool pacmanCaught = false;
		for (int g = 0; g < 3; g++)
		{
			if (ghostRows[g] == pacmanRow && ghostCols[g] == pacmanCol)
			{
				pacmanCaught = true;
				gameRunning = false;
				cout << "Game Over! Pacman was caught!\n";
				break;
			}
		}

		if (coinsCollected >= totalCoins)
		{
			gameRunning = false;
			cout << "You Win! All coins collected!\n";
		}

		if (gameRunning)
		{
			MovePacman();
			MoveGhosts();
		}
	}

	glutPostRedisplay();
}

void main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(400, 100);
	glutCreateWindow("Pacman AI vs AI");

	glutDisplayFunc(display);
	glutIdleFunc(idle);

	init();

	glutMainLoop();
}
