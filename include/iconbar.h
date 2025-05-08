#pragma once

#include "gridcad.h"


class icon {
public:
    icon(void);
    ~icon(void);
    
    class element *create_element(void);
private:

};

class iconbar {
public:
    iconbar(class scene *scene);
    ~iconbar(void);
private:
    std::vector<std::vector<class icon *>> icons;
};
