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
#include "element.h"

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
#include "model_deflipflop.h"
#include "model_delayline.h"
#include "model_clock.h"
#include "model_memory.h"
#include "model_label.h"
#include "model_buffer.h"
#include "model_const.h"
#include "model_add.h"
#include "model_sub.h"
#include "model_mux.h"
#include "model_reg.h"
#include "model_eq.h"
#include "model_ne.h"
#include "model_gt.h"
#include "model_ge.h"
#include "model_mem.h"
#include "model_slice.h"
#include "model_concat.h"
#include "model_split.h"
#include "connector.h"
#include "sequencer.h"

#define ELEMENT(x)  if (classid == #x":") return std::make_unique<x>(0, 0);
std::unique_ptr<class element> element_from_class_id(std::string_view classid)
{
    if (classid == "element:")
        return std::make_unique<element>(0,0, "");
        
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
    ELEMENT(model_deflipflop);
    ELEMENT(model_memory);
    ELEMENT(model_delayline);
    ELEMENT(model_clock);
    ELEMENT(model_label);
    ELEMENT(model_buffer);
    ELEMENT(model_const);
    ELEMENT(model_add);
    ELEMENT(model_sub);
    ELEMENT(model_mux);
    ELEMENT(model_reg);
    ELEMENT(model_eq);
    ELEMENT(model_ne);
    ELEMENT(model_gt);
    ELEMENT(model_ge);
    ELEMENT(model_mem);
    ELEMENT(model_slice);
    ELEMENT(model_concat);
    ELEMENT(model_split);
    ELEMENT(sequencer);
    
    
    logger::get().error("Unknown class_id: {}", classid);

    return nullptr;
}
