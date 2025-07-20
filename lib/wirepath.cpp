/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

#include "wirepath.h"
#include <algorithm>

#include <stdio.h>
#include <assert.h>
#include <math.h>

static int recursecount = 0;
static bool abort_all_walks = false;

wiregrid::wiregrid(int _width, int _height)
{
	int y;
	width = _width;
	height = _height;    
	grid.resize(_height);
	for (y = 0; y < _height; y++)
		grid[y].resize(width);

	/* start with an upper bound on best path */
	best_path = 3.0 * width * height;
	found_solution = false;
}

wiregrid::~wiregrid()
{
	int y;
	for (y = 0; y < height; y++)
		grid[y].resize(0);
	grid.resize(0);

}


void wiregrid::debug_display(int _X, int _Y)
{
	int x,y;
	int hqx = -1, hqy = -1;

	if (queue.size() > 0) {
		struct point *p = queue.top();
		hqx = p->x;
		hqy = p->y;
#if 0
		std::priority_queue<struct point *, std::vector<struct point *>, CompareTask> queue2;        
		queue2 = queue;
		while (queue2.size() > 0) {
			p = queue2.top();
			queue2.pop();
			printf("queue slot %i %i  at cost %5.2f\n", p->x, p->y, p->distance_from_start + p->hamdist);
		}
#endif 
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if (x == originX && y == originY) {
				printf("O");
				continue;
			}
			if (x == targetX && y == targetY) {
				printf("T");
				continue;
			}

			if (grid[y][x].on_path) {
				printf("*");
				continue;
			}

			if (grid[y][x].blocked) {
				printf("|");
				continue;
			}
#if 1
			if (grid[y][x].part_of_wire) {
				printf("=");
				continue;
			}
#endif
			if (grid[y][x].visited) {
				printf("x");
				continue;
			}
			if (x == hqx && y == hqy) {
				printf("Q");
				continue;
			}
			if (grid[y][x].queued) {
				printf("q");
				continue;
			}

			printf(".");
		}
		printf("\n");
	}
	printf("\n");
}

void wiregrid::block_point(int x, int y) 
{
	if (x < 0 || x >= width)
		return;
	if (y < 0 || y >= height)
		return;
	grid[y][x].blocked = true;
};
void wiregrid::declare_part_of_wire(int x, int y) 
{
	if (x < 0 || x >= width)
		return;
	if (y < 0 || y >= height)
		return;
	grid[y][x].part_of_wire = true;
};
void wiregrid::unblock_point(int x, int y) 
{
	if (x < 0 || x >= width)
		return;
	if (y < 0 || y >= height)
		return;
	grid[y][x].blocked =  false;
};

bool wiregrid::is_blocked(int x, int y)
{
	if (x < 0 || x >= width)
		return true;
	if (y < 0 || y >= height)
		return true;
	return grid[y][x].blocked;
}

float wiregrid::get_soft_cost(int x, int y)
{
	if (x < 0 || x >= width)
		return 0;
	if (y < 0 || y >= height)
		return 0;
	return grid[y][x].extra_score;
}

void wiregrid::add_soft_cost(int x, int y, double extra)
{
	if (x < 0 || x >= width)
		return;
	if (y < 0 || y >= height)
		return;
	if (extra >= 0)
		grid[y][x].extra_score += extra;
	else
		grid[y][x].extra_score = std::min(extra, grid[y][x].extra_score);

	/* negative values are allowed to encourage parallel routing etc -- but need to
	   be clipped to some relatively small amount or the algorithm becomes unstable
	   as very long routings can become cheap and loops etc will get encouraged */
	if (grid[y][x].extra_score < -0.1)
		grid[y][x].extra_score = -0.1;
}

/*
 * For the path finding algorithm we need a lower bound estimate
 * of the remaining distance. Traditionally one uses the pythagoras distance,
 * but for our case, since we only allow 0, 45 and 90 degree lines,
 * it's much simpler.
 */
double wiregrid::cost_estimate(int x, int y)
{
	int max, min;

	//    if (is_blocked(x, y) || x <0 || y < 0)
	//        return 4.0 * width * height;

	x -= targetX;
	y -= targetY;
	if (x < 0)
		x = -x;
	if (y < 0)
		y = -y;

	return sqrtf(x*x+y*y);
	if (x > y) {
		max = x;
		min = y;
	} else {
		max = y;
		min = x;
	}
	return SQRT2 * min + (max - min);
}


void wiregrid::update_destination(int tX, int tY)
{
	for (unsigned y = 0; y < grid.size(); y++)
		for (unsigned x = 0; x < grid[y].size(); x++) {
			grid[y][x].hamdist = cost_estimate(x,y);
			if ((int)x == tX && (int)y == tY)
				grid[y][x].hamdist = 0;
			grid[y][x].distance_from_start = 100000000.0;
			grid[y][x].x = x;
			grid[y][x].y = y;
			grid[y][x].visited = false;
			grid[y][x].queued = false;
		}
}


std::vector<struct waypoint> *  wiregrid::walk_back(void)
{
	int x, y;
	int prevDX = 0, prevDY = 0;

	std::vector<struct waypoint> *vec;

	vec = new std::vector<struct waypoint>;
	struct waypoint wp;

	if (abort_all_walks || !found_solution) {
		wp.X = targetX;
		wp.Y = targetY;
		vec->push_back(wp);
		wp.X = originX;
		wp.Y = originY;
		vec->push_back(wp);
		std::reverse(vec->begin(), vec->end());
		abort_all_walks = false;
		return vec;
	}

	x = targetX;
	y = targetY;

	while (true) {
		int dx, dy;
		if (grid[y][x].dirX == 0 && grid[y][x].dirY == 0)
			break;
		grid[y][x].part_of_wire = true;
		if (x == originX && y == originY) {
			break;
		}
		dx = grid[y][x].dirX;
		dy = grid[y][x].dirY;

		grid[y][x].on_path = true;

		if (dx != prevDX || dy != prevDY) {
			/* we're changing direction */
			wp.X = x;
			wp.Y = y;
			vec->push_back(wp);
		}

		prevDX = dx;
		prevDY = dy;
		x += dx;
		y += dy;
		if (x < 0 || x >= width)
			break;
		if (y <0 || y >= height)
			break;
	}
	wp.X = x;
	wp.Y = y;
	vec->push_back(wp);
	std::reverse(vec->begin(), vec->end());
	//   debug_display();
	return vec;
}

static const int DX[9] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
static const int DY[9] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};
static const float COST[9] = {SQRT2, 1, SQRT2, 1, 0, 1, SQRT2, 1, SQRT2 };
static const float COST2[9] = {1.4 * SQRT2, 1, 1.4 * SQRT2, 1, 0, 1, 1.4 * SQRT2, 1, 1.4 * SQRT2 };
static const int order[8] = {1,3,5,7,0,2,6,8};

void wiregrid::visit(struct point *p)
{
	recursecount++;
	//    debug_display();
	const float *EFFECTIVE_COST = COST;
	if (is_clock)
		EFFECTIVE_COST = COST2;
	if (p->x == targetX && p->y == targetY) {
		//        printf("Found the solution at distance %5.2f at rc %i\n", p->distance_from_start, recursecount);
		//        debug_display();
		found_solution = true;
		best_path = p->distance_from_start;       
		return;
	}

	p->visited = true;
	//    printf("Visiting %i %i  at distance %5.2f  hd %5.2f  qs %lu\n", p->x, p->y, p->distance_from_start, p->hamdist, queue.size());
	//    debug_display();
	for (unsigned int q = 0; q < 8; q++) {
		int i = order[q];
		int newX, newY;
		newX = p->x + DX[i];
		newY = p->y + DY[i];
		float newdist;

		if (newX < 0 || newY < 0 || newY >= height || newX >= width)
			continue;

		if (grid[newY][newX].blocked)
			continue;

		newdist = p->distance_from_start + p->extra_score + EFFECTIVE_COST[i];
		//        if (newX == targetX && newY == targetY)
		//            printf("SOLUTION best path is %5.2f  newdist is %5.2f   target %5.2f  t hd %5.2f\n", best_path, newdist, grid[newY][newX].distance_from_start, grid[newY][newX].hamdist);



		if (p->dirX != -DX[i] || p->dirY != -DY[i]) {
			if (is_clock)
				newdist += 0.3;
			newdist += 0.1;
		}
		//        printf("Looking at %i %i for newdist %5.2f\n", newX,newY, newdist);

		if (newdist + grid[newY][newX].hamdist > best_path + 1)
			continue;

		if (grid[newY][newX].distance_from_start > newdist) {
			grid[newY][newX].distance_from_start = newdist;
			grid[newY][newX].dirX = -DX[i];
			grid[newY][newX].dirY = -DY[i];
			if (!grid[newY][newX].queued) {
				queue.push(&grid[newY][newX]);
				grid[newY][newX].queued = true;
			}
		}
	}    

}


std::vector<struct waypoint> * wiregrid::path_walk(int x1, int y1, int x2, int y2, bool is_clock)
{
	if (x1 < 0)
		x1 = 0;
	if (x1 >= width)
		x1 = width -1;
	if (y1 < 0)
		y1 = 0;
	if (y1 >= height)
		y1 = height - 1;
	if (x2 < 0)
		x2 = 0;
	if (x2 >= width)
		x2 = width -1;
	if (y2 < 0)
		y2 = 0;
	if (y2 >= height)
		y2 = height - 1;
	originX = x1;
	originY = y1;
	targetX = x2;
	targetY = y2;

	abort_all_walks = false;   
	update_destination(targetX, targetY);

	recursecount = 0;
	found_solution = false;
	best_path = 3.0 * width * height;


	/* make sure the origin is not blocked -- which is quite possible as by default all terminal nodes
	 * are blocked */
	unblock_point(x1, y1);   
	unblock_point(x2, y2); 

	grid[originY][originX].distance_from_start = 0;

	queue.push(&grid[y1][x1]);


	while (queue.size() > 0) {
		struct point *p;
		p = queue.top();
		queue.pop();
		visit(p);
		p->queued = false;
	}   


//	printf("recursecount is %i for path length %5.2f \n", recursecount, best_path);
	//   debug_display();
	return walk_back();
}
