#include "gridcad.h"

#include "models.h"


#define ELEMENT(x)  if (classid == #x":") return new x(0, 0);
class element *element_from_class_id(std::string classid)
{
    if (classid == "element:")
        return new element(0,0, NULL);
        
    ELEMENT(model_toggle);
    ELEMENT(model_not);        
    ELEMENT(model_or);
    ELEMENT(model_nor);
    ELEMENT(model_and);
    ELEMENT(model_nand);
    ELEMENT(model_one);
    ELEMENT(model_zero);
    ELEMENT(connector);
    ELEMENT(model_output);
    
    
    printf("Unknown class_id: %s\n", classid.c_str());

    return nullptr;
}