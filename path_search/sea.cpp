#include "stdafx.h"
#include "sea.h"

#include <memory>
#include <set>
#include <list>
#include <array>

const std::string MAP_DIRECTORY = "maps";

// symbol codes
const uint8_t FREE_CELL = 45;
const uint8_t BUSY_CELL = 88;


struct PathPointNode
{
	using PathPointNodePtr = std::shared_ptr<PathPointNode>;

	static SeaPoint finish;

	PathPoint pos;
	PathPointNodePtr parent;
	int f_cost, g_cost, h_cost;

	PathPointNode(const PathPoint& pos_, const PathPointNodePtr& parent_)
		: pos(pos_), parent(parent_)
	{
		g_cost = parent->g_cost + (pos.turn ? 15 : 10);
		set_cost();
	}

	PathPointNode(const SeaPoint& p)	// init point
		: parent(nullptr)
	{
		pos.row = p.row;
		pos.col = p.col;
		g_cost = 0;
        set_cost();
	}

	PathPointNode(const PathPointNode&) = delete;
	PathPointNode& operator=(const PathPointNode&) = delete;

	void set_cost() {
		h_cost = 10*(std::abs(pos.row - finish.row) + std::abs(pos.col - finish.col));
		f_cost = g_cost + h_cost;

	}

	friend bool operator==(const PathPointNodePtr& p, const PathPointNodePtr& q) {
		return p->pos == q->pos;
	}

	friend bool operator==(const PathPointNodePtr& p, const PathPoint& q) {
		return p->pos == q;
	}

    bool try_update_cost(const PathPointNodePtr& prob_parent) {
        int new_g_cost = prob_parent->g_cost + (pos.turn ? 15 : 10);
        if (new_g_cost < g_cost) {
            //Log::Debug("Cost will be changed: " + to_string() + "; parent " + parent->to_string());
            g_cost = new_g_cost;
            f_cost = g_cost + h_cost;
            parent = prob_parent;
            //Log::Debug("Cost was changed: " + to_string() + "; parent " + parent->to_string());
            return true;
        }
        return false;
    }


    std::string to_string() {
        return pos.to_string() + " | " + std::to_string(f_cost) + " " +
            std::to_string(g_cost) + " " + std::to_string(h_cost);
    }
};
using PathPointNodePtr = PathPointNode::PathPointNodePtr;
SeaPoint PathPointNode::finish;

struct PathPointLess
{
	bool operator() (const PathPointNodePtr& p, const PathPointNodePtr& q) const {
		return p->f_cost < q->f_cost;
	}
};

class OpenList
{
    using ContType = std::multiset<PathPointNodePtr, PathPointLess>;
    ContType cont;
public:
    using IterType = ContType::iterator;

	bool empty() { return cont.empty(); }
    int size() { return cont.size(); }
    const IterType end() { return cont.end(); }
    
	void add(const PathPointNodePtr& p) {
		cont.insert(p);
	}

	PathPointNodePtr pop_least() {
		PathPointNodePtr ret = std::move(*cont.begin());
		cont.erase(cont.begin());
		return ret;
	}

	PathPointNodePtr find(const PathPointNodePtr& p) {
		auto eq = cont.equal_range(p);
		for (auto it = eq.first; it != eq.second; ++it)
			if (*it == p)
				return *it;
		return nullptr;
	}

    IterType find(const PathPoint& p) {
        for (auto it = cont.begin(); it != cont.end(); ++it)
            if (*it == p)
                return it;
        return cont.end();
    }

    void try_update_cost(IterType& it, const PathPointNodePtr& prob_parent) {
        if ((*it)->try_update_cost(prob_parent)) {
            PathPointNodePtr upd_obj = *it;
            cont.erase(it);
            cont.insert(upd_obj);
        }
    }
};

class ClosedList
{
	std::multimap<int, PathPointNodePtr> cont;

public:
	void add(const PathPointNodePtr& p) {
		cont.insert(std::make_pair(p->pos.row, p));
	}

	bool find(const PathPoint& p) {
		auto eq = cont.equal_range(p.row);
		for (auto it = eq.first; it != eq.second; ++it)
			if (it->second == p)
				return true;
		return false;
	}
};

//  here knowledge about the ship geometry
namespace Ship {
	bool check_init_place(const Sea* sea, int row, int col) {
		return sea->check_free(row, col) && sea->check_free(row - 1, col) && sea->check_free(row + 1, col);
	}
	
	/* check turns scheme
	1 - 2
	- X - 
	2 - 1
	*/
	bool check_turn1(const Sea* sea, const PathPoint& p) {
		return sea->check_free(p.row + 1, p.col - 1) && sea->check_free(p.row - 1, p.col + 1);
	}
	bool check_turn2(const Sea* sea, const PathPoint& p) {
		return sea->check_free(p.row + 1, p.col + 1) && sea->check_free(p.row - 1, p.col - 1);
	}

	void get_vtop(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		if (sea->check_free(p.row+2, p.col)) {
			adj = p;
			++adj.row;
			adj.turn = NONE_TURN;
		}
	}
	void get_vbottom(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		if (sea->check_free(p.row-2, p.col)) {
			adj = p;
			--adj.row;
			adj.turn = NONE_TURN;
		}
	}
	void get_vleft(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		bool t1 = check_turn1(sea, p);
		bool t2 = check_turn2(sea, p);
		if (sea->check_free(p.row, p.col - 2) && sea->check_free(p.row, p.col - 1) &&
			(t1 || t2) && sea->check_free(p.row, p.col + 1)) {
			adj.row = p.row;
			adj.col = p.col - 1;
			adj.vertical = false;
			adj.turn = t2 ? CLOCKWISE_TURN : ANTICLOCKWISE_TURN;
		}
	}
	void get_vright(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		bool t1 = check_turn1(sea, p);
		bool t2 = check_turn2(sea, p);
		if (sea->check_free(p.row, p.col + 2) && sea->check_free(p.row, p.col + 1) &&
			(t1 || t2) && sea->check_free(p.row, p.col - 1)) {
			adj.row = p.row;
			adj.col = p.col + 1;
			adj.vertical = false;
			adj.turn = t2 ? CLOCKWISE_TURN : ANTICLOCKWISE_TURN;
		}
	}

	void get_htop(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		bool t1 = check_turn1(sea, p);
		bool t2 = check_turn2(sea, p);
		if (sea->check_free(p.row + 2, p.col) && sea->check_free(p.row + 1, p.col) &&
			(t1 || t2) && sea->check_free(p.row - 1, p.col)) {
			adj.row = p.row + 1;
			adj.col = p.col;
			adj.vertical = true;
			adj.turn = t1 ? CLOCKWISE_TURN : ANTICLOCKWISE_TURN;
		}
	}
	void get_hbottom(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		bool t1 = check_turn1(sea, p);
		bool t2 = check_turn2(sea, p);
		if (sea->check_free(p.row - 2, p.col) && sea->check_free(p.row - 1, p.col) &&
			(t1 || t2) && sea->check_free(p.row + 1, p.col)) {
			adj.row = p.row - 1;
			adj.col = p.col;
			adj.vertical = true;
			adj.turn = t1 ? CLOCKWISE_TURN : ANTICLOCKWISE_TURN;
		}
	}
	void get_hleft(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		if (sea->check_free(p.row, p.col - 2)) {
			adj = p;
			adj.col--;
			adj.turn = NONE_TURN;
		}
	}
	void get_hright(const Sea* sea, const PathPoint& p, PathPoint& adj) {
		if (sea->check_free(p.row, p.col + 2)) {
			adj = p;
			adj.col++;
			adj.turn = NONE_TURN;
		}
	}

	std::array<PathPoint, 4> get_adjacent(const Sea* sea, const PathPoint& p) {
		std::array<PathPoint, 4> ret;
		if (p.vertical) {
			get_vtop(sea, p, ret[0]);
			get_vbottom(sea, p, ret[1]);
			get_vleft(sea, p, ret[2]);
			get_vright(sea, p, ret[3]);
		}
		else {
			get_htop(sea, p, ret[0]);
			get_hbottom(sea, p, ret[1]);
			get_hleft(sea, p, ret[2]);
			get_hright(sea, p, ret[3]);
		}
		return ret;
	}
}



Sea::Sea()
	: cells_loaded(false), path_calculated(false)
{
    Core::fileSystem.FindFiles(MAP_DIRECTORY+"/*", map_files);
    it_map_file = map_files.begin();
    if (it_map_file == map_files.end())
        Log::Error("No files into the maps directory");

    reload();
}

void Sea::reload()
{
	start.clear();
	finish.clear();
	path.clear();

    cells.clear();
	cells_loaded = false;

    if (it_map_file == map_files.end()) 
        return;

    curr_file_name = *it_map_file;
    IO::InputStreamPtr stream = Core::fileSystem.OpenRead(curr_file_name);
	next_map();
    if (!stream)
        return;

    std::vector<uint8_t> data;
    stream->ReadAllBytes(data);

    cells.emplace_back();
    for (const auto& sym : data) {
        if (sym == 10)
            cells.emplace_back();
        else
            cells.back().push_back(sym);
    }
    if (cells.size() > 0 && cells.back().size() == 0) // last empty line
        cells.pop_back();

	if (check_cells()) {
		cells_loaded = true;
		_width = cells[0].size();
		_height = cells.size();
	}              
}

bool Sea::check_cells() const
{
    if (cells.size() < 1)
        return false;

    size_t row_length = cells[0].size();
    if (row_length == 0)
        return false;

    for (const auto& row : cells) {
        if (row.size() != row_length)
            return false;
        for (const auto& sym : row) {
            if (sym != FREE_CELL && sym != BUSY_CELL)
                return false;
        }
    }
    return true;
}

std::string Sea::state() const
{
	if (!cells_loaded)
		return "Map is not loaded";
    
    return curr_file_name + std::string(" [") + std::to_string(width()) + "x" + std::to_string(height()) + "]";
}

void Sea::set_start(int row, int col)
{
	if (!Ship::check_init_place(this, row, col) || finish.equal(row, col))
		return;
	
	start.set(row, col);
	calculate_path();
}

void Sea::set_finish(int row, int col)
{
	if (!check_free(row, col) || start.equal(row, col))
		return;

	finish.set(row, col);
	calculate_path();
}

void Sea::walk_obstacles(const std::function<void(int, int)>& clb) const
{
	if (!cells_loaded)
		return;

	for (int r = 0; r < _width; ++r)
		for (int c = 0; c < _height; ++c)
			if (cells[r][c] == BUSY_CELL)
				clb(r, c);
}

bool Sea::check_free(int r, int c) const
{
	return check_inside(r, c) && cells[r][c] == FREE_CELL;
}

void Sea::calculate_path()
{
	if (start.empty() || finish.empty())
		return;
	
	path.clear();
	PathPointNode::finish = finish;
	OpenList open_list;
	open_list.add(std::make_shared<PathPointNode>(start));
	ClosedList closed_list;
	PathPointNodePtr route = nullptr;
	
	while (!open_list.empty() && !route) {
		auto curr = open_list.pop_least();
		closed_list.add(curr);
        //Log::Debug("curr: " + curr->to_string());

		auto adjacent_points = Ship::get_adjacent(this, curr->pos);
		for (auto& adj : adjacent_points) {
			if (adj.empty() || closed_list.find(adj))
				continue;

            OpenList::IterType point_iter = open_list.find(adj);
            if (point_iter != open_list.end()) {    // point exists
                open_list.try_update_cost(point_iter, curr);
			}
			else {
                auto new_point = std::make_shared<PathPointNode>(adj, curr);
				if (new_point->h_cost == 0) {	// Done!
					route = new_point;
					break;
				}
				open_list.add(new_point);
                //Log::Debug("add: " + adj_point->to_string());
                //Log::Debug("open_list size: " + std::to_string(open_list.size()));
			}
		}
	}

	while (route) {
		//Log::Debug(route->pos.to_string());
		path.push_front(route->pos);
		route = route->parent;
	}

	path_calculated = true;
}

void Sea::take_path(PathPointCollection& target_path) {
	if (path_calculated) {
		target_path.clear();
		path.swap(target_path);
		path_calculated = false;
	}
}
