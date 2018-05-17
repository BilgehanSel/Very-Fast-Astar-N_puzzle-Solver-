# Very-Fast-Astar-N_puzzle-Solver-
N puzzle solver for 3x3, 4x4, 5x5, 6x6 etc. By using A* heuristic algorithm
Why?
- Wanted to make a faster version of npuzzle solver by using unordered_set and priority_queue
- Improvements are; using two datasets for both fast access to best open_list member and also fast check for uniqueness on open_list
- Downside is; using 2 datasets, so double the memory needs; Or is it? I don't think after
experimenting that general_set is not using too much data;
