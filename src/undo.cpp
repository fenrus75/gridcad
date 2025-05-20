#include "gridcad.h"

#define UNDO_DEPTH 16



static std::vector<std::string> undo_list;

void take_undo_snapshot(class scene *scene)
{
	json J;
	std::string s;

	scene->to_json(J);

	s = J.dump();
	printf("Undo: %s \n", s.c_str());
	undo_list.push_back(s);
	if (undo_list.size() > UNDO_DEPTH)
		undo_list.erase(undo_list.begin() + 0);
}


class scene *get_undo(void)
{
	class scene *scene;
	std::string s;
	json J;

	if (undo_list.size() == 0)
		return NULL;

	scene = new class scene();
	s = undo_list[undo_list.size() -1];

	undo_list.pop_back();
	J = json::parse(s);
	scene->from_json(J);
	return scene;
}