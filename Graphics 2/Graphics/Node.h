#pragma once

class Node
{
private:
	int row;
	int col;
	Node* parent;
	double h, g, f;
public:
	Node();
	Node(int r, int c, Node* p, double g_val);
	void Draw();
	void ComputeH(int targetRow, int targetCol);
	void ComputeF();

	bool operator == (const Node &other);
	double getF() { return f; }
	double getH() { return h; }
	double getG() { return g; }
	int getRow() { return row; }
	int getCol() { return col; }
	Node* getParent() { return parent; }
	void setParent(Node* p) { parent = p; }
	void setG(double g_val) { g = g_val; ComputeF(); }
};

