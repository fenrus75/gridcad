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
#include "model_label.h"
#include "port.h"
#include "contextmenu.h"

#include <sys/time.h>

void callback_color(class element *elem)
{
    class model_label *label = (class model_label *) elem;
    int c = label->get_color();
    label->set_color(++c);    
}

model_label::model_label(float _X, float _Y)  : element(1, 1, "Label")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);

    name_edit = new class name(&name);
    name_edit->allow_spaces();
    
    menu->add_item("Edit name", callback_editname);
    menu->add_item("Change color", callback_color);
}

model_label::~model_label(void)
{
    delete name_edit;
}

void model_label::drawAt_early(class canvas *canvas, float X, float Y, int type)
{
    int col = color % 2;
    if (!selected)
      name_edit->set_edit_mode(false);
      
    if (selected) {
        if (col == 0)
            canvas->draw_image("assets/label_selected.png", X, Y, sizeX, sizeY, 255);
        if (col == 1)
            canvas->draw_image("assets/label2_selected.png", X, Y, sizeX, sizeY, 255);
    } else {
        if (col == 0)
            canvas->draw_image("assets/label.png", X, Y, sizeX, sizeY, 255);
        if (col == 1)
            canvas->draw_image("assets/label2.png", X, Y, sizeX, sizeY, 255);
        
    }

    if (bottomlabel)
        name_edit->drawAt(canvas, X, Y + sizeY-1.7, sizeX, 1.5);
    else
        name_edit->drawAt(canvas, X, Y + 0.2, sizeX, 1.5);
}



void model_label::to_json(json &j)
{
     element::to_json(j);
     j["color"] = color;
     j["bottomlabel"] = bottomlabel;
}
void model_label::from_json(json &j)
{
     element::from_json(j);
     color = j.value("color", 0);
     bottomlabel = j.value("bottomlabel", true);
}

void model_label::handle_event(class canvas *canvas, SDL_Event &event)
{
    if (!selected || !single)
        return;
    switch (event.type) {
	case SDL_KEYDOWN:        
        	switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    name_edit->toggle_edit_mode();
                    break;
                case SDLK_s:
                    if (!name_edit->get_edit_mode()) {
                        sizeX++;
                    }
                    break;
                case SDLK_a:
                    if (!name_edit->get_edit_mode()) {
                        if (sizeX > 3)
                            sizeX--;
                    }
                    break;
                case SDLK_z:
                    if (!name_edit->get_edit_mode()) {
                        sizeY++;
                    }
                    break;
                case SDLK_w:
                    if (!name_edit->get_edit_mode()) {
                        if (sizeY > 3)
                            sizeY--;
                    }
                    break;
                }
                break;
        }
    if (name_edit->get_edit_mode())
        name_edit->handle_event(event);
}


void model_label::draw_early(class canvas *canvas, int type)
{
    switch (type) {
        case DRAW_NORMAL:
            drawAt_early(canvas, X, Y, DRAW_NORMAL);
            break;
        case DRAW_GHOST:
            drawAt_early(canvas, Xghost, Yghost, DRAW_GHOST);
            break;
        case DRAW_DND:
            drawAt_early(canvas, Xdnd, Ydnd, DRAW_DND);
            break;
        case DRAW_ORIGIN:
            drawAt_early(canvas, X, Y, DRAW_ORIGIN);
            break;
    }
}

void model_label::update_drag(class canvas *canvas, class scene *scene, float _X, float _Y)
{
    Xdnd = _X - X_in_drag;
    Ydnd = _Y - Y_in_drag;
    
    float _Xghost, _Yghost;
    _Xghost = floorf(_X - X_in_drag + 0.25);
    _Yghost = floorf(_Y - Y_in_drag + 0.25);
    Xghost = _Xghost;
    Yghost = _Yghost;
}


void model_label::rotate_ports(void)
{
    bottomlabel = !bottomlabel;
}