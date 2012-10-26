#include "create_grid.h"
#include "grid.h"
#include <iostream>

namespace grid {

void add_corner (int id, Grid* m, int t1, int t2, int t3) {
	Corner *c = &m->corner[id];
	Tile *t[3] = {&m->tile[t1], &m->tile[t2], &m->tile[t3]};
	Vector3 v = t[0]->v + t[1]->v + t[2]->v;
	c->v = vector3::normal(v);
	for (int i=0; i<3; i++) {
		t[i]->corner[position(t[i], t[(i+2)%3])] = c;
		c->tile[i] = t[i];
	}
}

void add_edge (int id, Grid* m, int t1, int t2) {
	Edge *e = &m->edge[id];
	Tile *t[2] = {&m->tile[t1], &m->tile[t2]};
	Corner *c[2] = {t[0]->corner[position(t[0], t[1])], t[0]->corner[(position(t[0], t[1])+1)%t[0]->edges]};
	for (int i=0; i<2; i++) {
		t[i]->edge[position(t[i], t[(i+1)%2])] = e;
		e->tile[i] = t[i];
		c[i]->edge[position(c[i], c[(i+1)%2])] = e;
		e->corner[i] = c[i];
	}
}

Grid* create (int size) {
	Grid* m = nullptr;
	if (size > 0) {
		Grid* prev = create(size-1);
		m = subdivision(prev);
		delete prev;
	}
	else {
		m = icosahedron();
	}
	return m;
}

Grid* icosahedron () {
	double x = 0.525731112119133606;
	double z = 0.850650808352039932;
	
	Vector3 icos_tile[12] = {
		Vector3(-x, 0, z), Vector3(x, 0, z), Vector3(-x, 0, -z), Vector3(x, 0, -z),
		Vector3(0, z, x), Vector3(0, z, -x), Vector3(0, -z, x), Vector3(0, -z, -x),
		Vector3(z, x, 0), Vector3(-z, x, 0), Vector3(z, -x, 0), Vector3(-z, -x, 0)
	};
	
	int icos_tile_n[12][5] = {
		{9, 4, 1, 6, 11}, {4, 8, 10, 6, 0}, {11, 7, 3, 5, 9}, {2, 7, 10, 8, 5},
		{9, 5, 8, 1, 0}, {2, 3, 8, 4, 9}, {0, 1, 10, 7, 11}, {11, 6, 10, 3, 2},
		{5, 3, 10, 1, 4}, {2, 5, 4, 0, 11}, {3, 7, 6, 1, 8}, {7, 2, 9, 0, 6}
	};

	Grid* m = new Grid();
	init(m, 0);
	
	for (int i=0; i<12; i++) {
		m->tile[i].v = icos_tile[i];
		for (int k=0; k<5; k++) {
			m->tile[i].tile[k] = &m->tile[icos_tile_n[i][k]];
		}
	}
	for (int i=0; i<5; i++) {
		add_corner(i, m, 0, icos_tile_n[0][(i+4)%5], icos_tile_n[0][i]);
	}
	for (int i=0; i<5; i++) {
		add_corner(i+5, m, 3, icos_tile_n[3][(i+4)%5], icos_tile_n[3][i]);
	}
	add_corner(10,m,10,1,8);
	add_corner(11,m,1,10,6);
	add_corner(12,m,6,10,7);
	add_corner(13,m,6,7,11);
	add_corner(14,m,11,7,2);
	add_corner(15,m,11,2,9);
	add_corner(16,m,9,2,5);
	add_corner(17,m,9,5,4);
	add_corner(18,m,4,5,8);
	add_corner(19,m,4,8,1);
	
	//add corners to corners
	for (int i=0; i<m->corners; i++) {
		Corner *c = &m->corner[i];
		for (int k=0; k<3; k++) {
			c->corner[k] = c->tile[k]->corner[(position(c->tile[k], c)+1)%5];
		}
	}
	//new edges
	int e_id = 0;
	for (int i=0; i<12; i++) {
		for (int k=0; k<5; k++) {
			if (m->tile[i].edge[k] == nullptr) {
				add_edge(e_id, m, i, icos_tile_n[i][k]);
				e_id++;
			}
		}
	}
	return m;
}

Grid* subdivision (const Grid* prev_grid) {
	Grid* grid = new Grid();
	init(grid, prev_grid->size+1);
	//old tiles
	for (int i=0; i<prev_grid->tiles; i++) {
		grid->tile[i].v = prev_grid->tile[i].v;
		for (int k=0; k<grid->tile[i].edges; k++) {
			grid->tile[i].tile[k] = &grid->tile[prev_grid->tile[i].corner[k]->id+prev_grid->tiles];
		}
	}
	//old corners become tiles
	for (int i=0; i<prev_grid->corners; i++) {
		grid->tile[i+prev_grid->tiles].v = prev_grid->corner[i].v;
		for (int k=0; k<3; k++) {
			grid->tile[i+prev_grid->tiles].tile[2*k] = &grid->tile[prev_grid->corner[i].corner[k]->id+prev_grid->tiles];
			grid->tile[i+prev_grid->tiles].tile[2*k+1] = &grid->tile[prev_grid->corner[i].tile[k]->id];
		}
	}
	//new corners
	int c_id = 0;
	for (int i=0; i<prev_grid->tiles; i++) {
		int n = grid->tile[i].edges;
		for (int k=0; k<n; k++) {
			add_corner(c_id, grid, i, grid->tile[i].tile[(k+n-1)%n]->id, grid->tile[i].tile[k]->id);
			c_id++;
		}
	}
	//add corners to corners
	for (int i=0; i<grid->corners; i++) {
		Corner *c = &grid->corner[i];
		for (int k=0; k<3; k++) {
			c->corner[k] = c->tile[k]->corner[(position(c->tile[k], c)+1)%(c->tile[k]->edges)];
		}
	}
	//new edges
	int e_id = 0;
	for (int i=0; i<grid->tiles; i++) {
		for (int k=0; k<grid->tile[i].edges; k++) {
			if (grid->tile[i].edge[k] == nullptr) {
				add_edge(e_id, grid, i, grid->tile[i].tile[k]->id);
				e_id++;
			}
		}
	}
	return grid;
}

int validate (const Grid* m) {
	bool valid = true;
	for (int i=0; i<m->tiles; i++) {
		const Tile* t = &m->tile[i];
		for (int k=0; k<t->edges; k++) {
			if (position(t->tile[k], t) == -1) {
				valid = false;
				std::cout << "error: tile " << t->id << ", tile " << k << std::endl;
			}
			if (position(t->corner[k], t) == -1) {
				valid = false;
				std::cout << "error: tile " << t->id << ", corner " << k << std::endl;
			}
			if (sign(t->edge[k], t) == 0) {
				valid = false;
				std::cout << "error: tile " << t->id << ", edge " << k << std::endl;
			}
		}
	}
	for (int i=0; i<m->corners; i++) {
		const Corner* c = &m->corner[i];
		for (int k=0; k<3; k++) {
			if (position(c->corner[k], c) == -1) {
				valid = false;
				std::cout << "error: corner " << c->id << ", corner " << k << std::endl;
			}
			if (sign(c->edge[k], c) == 0) {
				valid = false;
				std::cout << "error: corner " << c->id << ", edge " << k << std::endl;
			}
		}
	}
	for (int i=0; i<m->edges; i++) {
		const Edge* e = &m->edge[i];
		for (int k=0; k<2; k++) {
			if (position(e->tile[k], e) == -1) {
				valid = false;
				std::cout << "error: edge " << e->id << ", tile " << k << std::endl;
			}
			if (position(e->corner[k], e) == -1) {
				valid = false;
				std::cout << "error: edge " << e->id << ", corner " << k << std::endl;
			}
		}
	}
	if (valid) std::cout << "grid valid\n";
	else std::cout << "grid invalid, abandon ship\n";
	return !valid;
}

}