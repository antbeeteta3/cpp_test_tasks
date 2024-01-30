#pragma once

#ifndef __SEA_H__
#define __SEA_H__

#include <vector>
#include <deque>
#include <string>
#include <functional>


struct SeaPoint
{
	int row;
	int col;

	SeaPoint(int row_, int col_) : row(row_), col(col_) {}
	SeaPoint() : row(-1), col(-1) {}

	bool empty() const { 
		return row < 0 || col < 0; 
	}
	void set(int row_, int col_) { 
		row = row_;
		col = col_; 
	}
	void clear() {
		row = -1;
		col = -1;
	}

	bool operator==(const SeaPoint& oth) {
		return oth.row == row && oth.col == col;
	}
	bool equal(int r, int c) {
		return r == row && c == col;
	}
};

enum TurnType
{
	NONE_TURN,
	CLOCKWISE_TURN,
	ANTICLOCKWISE_TURN
};

struct PathPoint : public SeaPoint
{
	bool vertical;
	TurnType turn;

	PathPoint() :
		SeaPoint(), vertical(true), turn(NONE_TURN) {}

	bool operator== (const PathPoint& oth) const {
		return row == oth.row && col == oth.col && vertical == oth.vertical;
	}

	std::string to_string() {
		return std::to_string(row) + " " + std::to_string(col) + " " + 
			std::to_string(vertical) + " " + std::to_string(turn);
	}
};

using PathPointCollection = std::deque<PathPoint>;

class Sea {
public:
    Sea();

    void reload();
	bool loaded() const { return cells_loaded; }

	std::string state() const;

	int width() const {
		if (cells_loaded)
			return _width;
		return 0;
	}
	int height() const {
		if (cells_loaded)
			return _height;
		return 0;
	}

	void set_start(int row, int col);
	const SeaPoint& get_start() const { return start; }
	void set_finish(int row, int col);
	const SeaPoint& get_finish() const { return finish; }
    bool limits_ready() { return !start.empty() && !finish.empty(); }

	void walk_obstacles(const std::function<void(int, int)>& clb) const;
	inline bool check_free(int r, int c) const;

	bool path_ready() const { return path_calculated; }
	void take_path(PathPointCollection& target_path);
			
private:
    std::vector<std::string> map_files;
    std::vector<std::string>::iterator it_map_file;
    void next_map() {
        if (++it_map_file == map_files.end())
            it_map_file = map_files.begin();
    }
    std::string curr_file_name;

    std::vector<std::vector<uint8_t>> cells;
	int _width = 0;
	int _height = 0;
    bool cells_loaded;
    bool check_cells() const;     // check after loading

	SeaPoint start;
	SeaPoint finish;

	bool check_inside(int r, int c) const {
		return cells_loaded && r >= 0 && r < _width && c >= 0 && c < _height;
	}
	
	void calculate_path();
    PathPointCollection path;
	bool path_calculated;
};

#endif // __SEA_H__