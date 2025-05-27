#include "gridcad.h"

#include "model_toggle.h"
#include "model_not.h"
#include "model_or.h"
#include "model_xor.h"
#include "model_nor.h"
#include "model_and.h"
#include "model_nand.h"
#include "model_one.h"
#include "model_zero.h"
#include "model_output.h"
#include "model_truth.h"
#include "model_nest.h"
#include "model_4to1.h"
#include "connector.h"

#define ELEMENT(x)  if (classid == #x":") return new x(0, 0);
class element *element_from_class_id(std::string classid)
{
    if (classid == "element:")
        return new element(0,0, "");
        
    ELEMENT(model_toggle);
    ELEMENT(model_not);        
    ELEMENT(model_or);
    ELEMENT(model_xor);
    ELEMENT(model_nor);
    ELEMENT(model_and);
    ELEMENT(model_nand);
    ELEMENT(model_one);
    ELEMENT(model_zero);
    ELEMENT(connector);
    ELEMENT(model_output);
    ELEMENT(model_truth);
    ELEMENT(model_nest);
    ELEMENT(model_4to1);
    
    
    printf("Unknown class_id: %s\n", classid.c_str());

    return nullptr;
}
