#pragma once
const int BSZ = 4;
const int UP = 1;
const int DOWN = 2;
const int LEFT = 3;
const int RIGHT = 4;

class Node
{
private:
	int board[BSZ][BSZ];
	int emptyRow;
	int emptyCol;
	Node* parent;
	double h, g, f;
public:
	Node();
	Node(Node* pn, int direction);
	void Draw();
	void ComputeH();
	void ComputeF();

	int ManhattanDistance(int row, int col);
	void ExchangeCells(int row, int col);

	bool operator == (const Node &other);
	double getF() { return f; }
	double getH() { return h; }
	int getEmptyRow() { return emptyRow; }
	int getEmptyCol() { return emptyCol; }
};

