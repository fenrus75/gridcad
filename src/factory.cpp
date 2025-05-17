#include "gridcad.h"



class element *element_from_class_id(std::string classid)
{
    if (classid == "element:")
        return new element(0,0, NULL);

    return nullptr;
}