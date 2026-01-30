#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "glut.h"

#include <vector>
#include <list>
#include <queue>
#include <utility>
#include <iostream>
#include <algorithm>
#include <functional>

using namespace std;

const int MSZ = 50;
const int SPACE = 0;
const int WALL = 1;
const int COIN = 2;
const int PACMAN = 3;
const int GHOST = 4;

const int MAX_BFS_DEPTH = 5; // Limited depth for BFS
const int MOVE_INTERVAL_MS = 250; // milliseconds between moves (slower = higher value)
const int MIN_GHOST_DISTANCE = 6; // ghosts spawn at least this many steps from Pacman

int maze[MSZ][MSZ] = { 0 };
int lastMoveTime = 0; // for throttling movement
int pacmanRow, pacmanCol;
int prevPacmanRow = -1, prevPacmanCol = -1; // avoid fleeing back and forth
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
vector<pair<int, int>> GetReachableSpaceCells(int fromRow, int fromCol);
vector<pair<int, int>> AStar(int startRow, int startCol, int targetRow, int targetCol);
int LimitedBFS(int startRow, int startCol, int maxDepth);
int FindNearestCoin(int row, int col);
int GetDirectionAwayFromGhost(int pacmanRow, int pacmanCol, int ghostRow, int ghostCol, int prevRow = -1, int prevCol = -1);
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

	// Place 3 ghosts: reachable from Pacman and at least MIN_GHOST_DISTANCE steps away
	vector<pair<int, int>> reachable = GetReachableSpaceCells(pacmanRow, pacmanCol);
	vector<pair<int, int>> farEnough;
	for (size_t i = 0; i < reachable.size(); i++)
	{
		int r = reachable[i].first, c = reachable[i].second;
		int dist = abs(r - pacmanRow) + abs(c - pacmanCol);
		if (dist >= MIN_GHOST_DISTANCE)
			farEnough.push_back(reachable[i]);
	}
	if (farEnough.empty())
		farEnough = reachable; // fallback if maze is small
	for (int g = 0; g < 3; g++)
	{
		if (farEnough.empty())
			break;
		int idx = rand() % (int)farEnough.size();
		ghostRows[g] = farEnough[idx].first;
		ghostCols[g] = farEnough[idx].second;
		maze[ghostRows[g]][ghostCols[g]] = GHOST;
		farEnough.erase(farEnough.begin() + idx);
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

// BFS from (fromRow, fromCol); returns all SPACE cells reachable without crossing walls.
vector<pair<int, int>> GetReachableSpaceCells(int fromRow, int fromCol)
{
	vector<pair<int, int>> result;
	queue<pair<int, int>> q;
	vector<vector<bool>> visited(MSZ, vector<bool>(MSZ, false));

	q.push({ fromRow, fromCol });
	visited[fromRow][fromCol] = true;

	int directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

	while (!q.empty())
	{
		int r = q.front().first;
		int c = q.front().second;
		q.pop();

		if (maze[r][c] == SPACE)
			result.push_back({ r, c });

		for (int d = 0; d < 4; d++)
		{
			int nr = r + directions[d][0];
			int nc = c + directions[d][1];
			if (nr >= 0 && nr < MSZ && nc >= 0 && nc < MSZ &&
				!visited[nr][nc] && maze[nr][nc] != WALL)
			{
				visited[nr][nc] = true;
				q.push({ nr, nc });
			}
		}
	}
	return result;
}

// A* using (f, row, col) in pq - no pointers, so no dangling refs or duplicate-pop bugs
vector<pair<int, int>> AStar(int startRow, int startCol, int targetRow, int targetCol)
{
	// min-heap by f: (f, (row, col))
	typedef pair<double, pair<int, int>> PQEntry;
	priority_queue<PQEntry, vector<PQEntry>, greater<PQEntry>> pq;

	vector<vector<double>> g(MSZ, vector<double>(MSZ, 1e9));
	vector<vector<pair<int, int>>> parent(MSZ, vector<pair<int, int>>(MSZ, { -1, -1 }));
	vector<vector<bool>> closed(MSZ, vector<bool>(MSZ, false));

	auto h = [targetRow, targetCol](int r, int c) {
		return (double)(abs(r - targetRow) + abs(c - targetCol));
	};

	g[startRow][startCol] = 0;
	double f0 = 0 + h(startRow, startCol);
	pq.push({ f0, { startRow, startCol } });

	int dr[4] = { -1, 1, 0, 0 };
	int dc[4] = { 0, 0, -1, 1 };

	while (!pq.empty())
	{
		double f = pq.top().first;
		int row = pq.top().second.first;
		int col = pq.top().second.second;
		pq.pop();

		// Stale entry: we already closed this cell with a better f
		if (closed[row][col])
			continue;
		closed[row][col] = true;

		if (row == targetRow && col == targetCol)
		{
			vector<pair<int, int>> path;
			int r = row, c = col;
			while (r >= 0 && c >= 0)
			{
				path.push_back({ r, c });
				int pr = parent[r][c].first, pc = parent[r][c].second;
				r = pr; c = pc;
			}
			reverse(path.begin(), path.end());
			return path;
		}

		for (int d = 0; d < 4; d++)
		{
			int nr = row + dr[d];
			int nc = col + dc[d];
			if (nr < 0 || nr >= MSZ || nc < 0 || nc >= MSZ || maze[nr][nc] == WALL)
				continue;
			if (closed[nr][nc])
				continue;
			double ng = g[row][col] + 1;
			if (ng < g[nr][nc])
			{
				g[nr][nc] = ng;
				parent[nr][nc] = { row, col };
				double nf = ng + h(nr, nc);
				pq.push({ nf, { nr, nc } });
			}
		}
	}
	return vector<pair<int, int>>(); // no path
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

int GetDirectionAwayFromGhost(int pacmanRow, int pacmanCol, int ghostRow, int ghostCol, int prevRow, int prevCol)
{
	int bestDir = UP;
	double maxDist = -1;
	int fallbackDir = UP;
	double fallbackDist = -1;

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
			bool isPrevCell = (prevRow >= 0 && prevCol >= 0 && newRow == prevRow && newCol == prevCol);
			if (isPrevCell)
			{
				if (dist > fallbackDist)
				{
					fallbackDist = dist;
					fallbackDir = dirNames[d];
				}
			}
			else if (dist > maxDist)
			{
				maxDist = dist;
				bestDir = dirNames[d];
			}
		}
	}
	// Prefer direction that is not the previous cell; only go back if no other option
	return (maxDist >= 0) ? bestDir : fallbackDir;
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
		// Ghost found - move away from nearest ghost (avoid going back to previous cell)
		int dir = GetDirectionAwayFromGhost(pacmanRow, pacmanCol, 
			ghostRows[nearestGhostIdx], ghostCols[nearestGhostIdx], prevPacmanRow, prevPacmanCol);
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
		prevPacmanRow = pacmanRow;
		prevPacmanCol = pacmanCol;
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
		vector<pair<int, int>> path = AStar(ghostRows[g], ghostCols[g], pacmanRow, pacmanCol);

		if (path.size() > 1)
		{
			// Move to next step in path
			int newRow = path[1].first;
			int newCol = path[1].second;

			if (newRow >= 0 && newRow < MSZ && newCol >= 0 && newCol < MSZ &&
				maze[newRow][newCol] != WALL)
			{
				if (maze[newRow][newCol] == COIN)
					totalCoins--; // ghost stepped on coin, so one fewer coin to collect
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
			int now = glutGet(GLUT_ELAPSED_TIME);
			if (now - lastMoveTime >= MOVE_INTERVAL_MS)
			{
				lastMoveTime = now;
				MovePacman();
				MoveGhosts();
			}
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
