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
#include "model_1to4.h"
#include "model_8to1.h"
#include "model_1to8.h"
#include "model_8to4.h"
#include "model_4to8.h"
#include "model_16to8.h"
#include "model_8to16.h"
#include "model_datascope.h"
#include "model_dflipflop.h"
#include "model_delayline.h"
#include "model_clock.h"
#include "model_memory.h"
#include "model_label.h"
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
    ELEMENT(model_1to4);
    ELEMENT(model_8to1);
    ELEMENT(model_1to8);
    ELEMENT(model_8to4);
    ELEMENT(model_4to8);
    ELEMENT(model_16to8);
    ELEMENT(model_8to16);
    ELEMENT(model_datascope);
    ELEMENT(model_dflipflop);
    ELEMENT(model_memory);
    ELEMENT(model_delayline);
    ELEMENT(model_clock);
    ELEMENT(model_label);
    
    
    printf("Unknown class_id: %s\n", classid.c_str());

    return nullptr;
}
