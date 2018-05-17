/*	Npuzzle solver with A* algorithm
	Date: 5/16/2018
	Why?
		- Wanted to make a faster version of npuzzle solver by using unordered_set and priority_queue
		- Improvements are; using two datasets for both fast access to best open_list member and also fast check for uniqueness on open_list
		- Downside is; using 2 datasets, so double the memory needs; Or is it? I don't think after experimenting that general_set is not using too much data;


	To use the program, only creating a new Npuzzle object and passing the puzzle you want the solution to and running Npuzzle_object.Solve() is enough() .
	Possible bug is that, entering already solved puzzle into the object...
	I would suggest compiling this to x64, since x86's ram usage may be insufficient.
	

	- If you have any improvements on the project send an email to bilgehansel@gmail.com
	- One possible improvement would be speeding up the deletion of nodes that was created in the ~Npuzzle destructor, which I don't know how...
	Written by Bilgehan Sel
*/

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <chrono>
#include <unordered_set>
#include <thread>
#include <list>

float distanceWeight = 1.0f;
float depthWeight = 0.5f; // Be aware if you make depthWeight big, your size might be really big... I would advide between 0.2 - 0.5

//std::vector<std::vector<char>> global_goal_state = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 0 } }; // Goal state for 8 puzzle
//std::vector<std::vector<char>> global_goal_state = { { 1, 2, 3, 4 },{ 5, 6, 7, 8 },{ 9, 10, 11, 12 },{ 13, 14, 15, 0 } }; // Goal state for 15 puzzle
std::vector<std::vector<char>> global_goal_state = { { 1, 2, 3, 4, 5 }, { 6, 7, 8, 9, 10 }, { 11, 12, 13, 14, 15 }, { 16, 17, 18, 19, 20 }, { 21, 22, 23, 24, 0 } }; // Goal state for 24 puzzle
//std::vector<std::vector<char>> global_goal_state = { { 1, 2, 3, 4, 5, 6 },{ 7, 8, 9, 10, 11, 12 },{ 13, 14, 15, 16, 17, 18 },{ 19, 20, 21, 22, 23, 24 },{ 25, 26, 27, 28, 29, 30 },{ 31, 32, 33, 34, 35, 0 } }; // Goal state for 35 puzzle


// object that has all properties that a generated puzzle would need.
struct node
{
	node() {}
	node(const std::vector<std::vector<char>>& p, unsigned d)
	{
		puzzle = p;
		depth = d;
		// Initializing cost
		int distance = 0;
		for (unsigned i = 0; i < puzzle.size(); i++) {
			for (unsigned j = 0; j < puzzle.size(); j++) {
				if (p[i][j] != 0) {
					if (p[i][j] % puzzle.size() != 0) {
						int temp_i = int(p[i][j] / p.size());
						int temp_j = int(p[i][j] % p.size() - 1);
						distance += std::abs(temp_i - int(i)) + std::abs(temp_j - int(j));
					}
					else {
						int temp_i = int(p[i][j] / p.size() - 1);
						int temp_j = int(p.size() - 1);
						distance += std::abs(temp_i - int(i)) + std::abs(temp_j - int(j));
					}
				}
				else {
					distance += std::abs(int(puzzle.size()) - 1 - int(i)) + std::abs(int(puzzle.size()) - 1 - int(j));
				}
				//puzzle_str += std::to_string(puzzle[i][j]);
			}
		}


		//std::cout << "puzzle_str: " << puzzle_str << std::endl;
		//std::cout << "distance is: " << distance << std::endl;
		cost = int(distance * distanceWeight) + int(depth * depthWeight);
		//std::cout << "cost is: " << cost << std::endl;
	}

	// Getting puzzle's string format since we'll need it for the hash table(unordered_set) to check whether a generated puzzle was already inside general_set
	std::string GetPuzzleStr() const {
		std::string puzzle_str = "";
		for (unsigned i = 0; i < puzzle.size(); i++) {
			for (unsigned j = 0; j < puzzle.size(); j++) {
				puzzle_str += std::to_string(puzzle[i][j]);
			}
		}
		return puzzle_str;
	}

	
	std::vector<std::vector<char>> puzzle; // puzzle is stored here
	
	char zero_pos[2]; // for finding 0 or empty tile inside the puzzle object (for speed)
	unsigned short cost;
	unsigned short depth; //it's depth, root(start_state) has zero(0) depth
	
	node* parent; // The linkage to its parent since we'll need to go back to the start_state when we find solution...
};

// Comparator for the priority queue, since it does need a way to prioritize objects it stores, we prioritize them in ascending order of object's cost property
class Comparator {
public:
	bool operator()(const node* a, const node* b)
	{
		return (a->cost > b->cost);
	}
};

// for std::cout'ing our puzzle type which is std::vector<std::vector<char>>
template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<std::vector<T>> v) {
	for (unsigned i = 0; i != v.size(); i++) {
		for (unsigned j = 0; j != v.size(); j++) {
			stream << int(v[i][j]) << " ";
		}
		stream << std::endl;
	}
	stream << std::endl;
	return stream;
}



class Npuzzle
{
public:
	Npuzzle() {}; // Default Constructor
	// Initializing Npuzzle obj by the given puzzle type
	Npuzzle(const std::vector<std::vector<char>>& p)
	{
		start_state = p;
		goal_state = global_goal_state;
		node* root = new node(p, 0);
		root->parent = nullptr;
		// Initializing zeropos, since we need it to find 0(empty tile) but we only need this once, since later generated nodes will already know their zeropos[].
		for (unsigned i = 0; i != p.size(); i++) {
			for (unsigned j = 0; j != p.size(); j++) {
				if (p[i][j] == 0) {
					root->zero_pos[0] = i;
					root->zero_pos[1] = j;
				}
			}
		}
		open_list.push(root);
		general_set.insert(root->GetPuzzleStr());
	}

	// Destructor to delete all those nodes that are stored inside open_list and closed_list
	~Npuzzle()
	{	
		std::thread t1([this]() {
			for (auto& v : closed_list)
				delete v;

			std::cout << "Closed_list has been cleaned..." << std::endl;
		});

		delete solved;
		while (!open_list.empty()) {
			delete open_list.top();
			open_list.pop();
		}
		std::cout << "Open_list has been cleaned..." << std::endl;

		// Wait for t1 to finish cleanup
		t1.join();
	}


	// Chosing the best(with the lowest cost) node and expanding(GenChildren) it, until a solution is found...
	void Solve()
	{
		while (!open_list.empty())
		{
			current = open_list.top();
			open_list.pop();
			closed_list.push_back(current);
			GenChildren();
	
			if (Done) {
				std::cout << "it's done." << std::endl;
				DisplaySolution();
				//std::cout << "General Set Size is: " << general_set.size() << "\tin Bytes: " << general_set.size() << std::endl;
				std::cout << "Open List size: " << open_list.size() << "\tClosed List size: " << closed_list.size() << std::endl;
				std::cout << "Depth is: " << solved->depth << std::endl;
				break;
			}
		}
	}

	// Displaying all the solution steps
	void DisplaySolution() const {
		node* temp = solved;
		do
		{
			std::cout << temp->puzzle;
			temp = temp->parent;
		} while (temp != nullptr);
	}

private:
	void GenChildren()
	{
		// Generating new puzzle with swapping values with zero, 4 directions, RIGHT, LEFT, UP, DOWN.
		// Also checking for bounds of the vector
		unsigned i = current->zero_pos[0];
		unsigned j = current->zero_pos[1];
		if (current->puzzle[i][j] == 0) {
			// RIGHT
			if (j + 1 < current->puzzle.size()) {
				std::vector<std::vector<char>> temp = current->puzzle;
				std::swap(temp[i][j + 1], temp[i][j]);
				GenNode(temp, i, j + 1);
			}
			// LEFT
			if (j > 0) {
				std::vector<std::vector<char>> temp = current->puzzle;
				std::swap(temp[i][j - 1], temp[i][j]);
				GenNode(temp, i, j - 1);
			}
			// UP
			if (i > 0) {
				std::vector<std::vector<char>> temp = current->puzzle;
				std::swap(temp[i - 1][j], temp[i][j]);
				GenNode(temp, i - 1, j);
			}
			// DOWN
			if (i + 1 < current->puzzle.size()) {
				std::vector<std::vector<char>> temp = current->puzzle;
				std::swap(temp[i + 1][j], temp[i][j]);
				GenNode(temp, i + 1, j);
			}
		}
	}

	// Generating node object and initiating its needed values
	void GenNode(std::vector<std::vector<char>>& temp, unsigned zero_pos0, unsigned zero_pos1)
	{
		if (temp == goal_state) {
			solved = new node(temp, current->depth + 1);
			solved->parent = current;
			Done = true;
			return;
		}
		else {
			node* temp_node = new node(temp, current->depth + 1);
			std::string puzzle_str = temp_node->GetPuzzleStr();
			if (general_set.find(puzzle_str) == general_set.end()) {
				temp_node->parent = current;
				temp_node->zero_pos[0] = zero_pos0;
				temp_node->zero_pos[1] = zero_pos1;
				open_list.push(temp_node);
				general_set.insert(puzzle_str);
			}
			else {
				delete temp_node;
			}
		}
	}


	std::priority_queue<node*, std::vector<node*>, Comparator> open_list; // our list to keep track of the nodes we still didn't expand yet
	std::list<node*> closed_list;// simple std::vector<node*> can be used also
	std::unordered_set<std::string> general_set;

	std::vector<std::vector<char>> start_state;
	std::vector<std::vector<char>> goal_state;

	node* current;
	node* solved = nullptr; // When solution is found, this changes to the solved node, then iterating through its parent(solved->parent->parent...) to display all solutions


	bool Done = false; // Whether a solution is found...
};

int main() {
	//Examples for start_puzzle....
	//std::vector<std::vector<char>> start_puzzle = { { 8, 6, 7 },{ 2, 5, 4 },{ 3, 0, 1 } };
	//std::vector<std::vector<int>> start_puzzle = { { 1, 3, 6 },{ 5, 2, 8 },{ 4, 0, 7 } };
	//std::vector<std::vector<int>> start_puzzle = { { 1, 3, 0 },{ 4, 2, 6 },{ 7, 5, 8 } };
	//std::vector<std::vector<int>> start_puzzle = { { 1, 4, 14, 7 },{ 6, 11, 2, 8 },{ 5, 9, 13, 10 },{ 12, 15, 3, 0 } };
	std::vector<std::vector<char>> start_puzzle = { { 1, 5, 2, 7, 10 },{ 6, 3, 15, 11, 0 },{ 22, 12, 16, 4, 9 },{ 17, 23, 24, 13, 18 },{ 21, 14, 8, 20, 19 } };
	//std::vector<std::vector<char>> start_puzzle = { { 2, 23, 16, 22, 34, 5 }, { 30, 32, 10, 0, 12, 17 }, { 4, 6, 8, 24, 26, 21 }, { 7, 3, 11, 29, 19, 33 }, { 14, 20, 25, 18, 1, 9 }, { 27, 15, 35, 28, 13, 31 } };
	
	// Testing how the code runs, put your code inside *start* and *end* variables 
	auto start = std::chrono::high_resolution_clock::now();

	// Code here to test its speed...
	Npuzzle init(start_puzzle);
	init.Solve();

	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "\n\nExecution time is: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms." << std::endl;


	std::cin.get();
	return 0;
}
