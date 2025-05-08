#include "gridcad.h"
#include "iconbar.h"


iconbar::iconbar(class scene *scene)
{
    icons.resize(2);
    
    for (auto icon : icons)
        icon.resize(10);
}

iconbar::~iconbar(void)
{
}
