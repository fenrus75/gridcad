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
#include "widget.h"


widget::widget(float X, float Y, float W, float H)
{
    X1 = X;
    Y1 = Y;
    X2 = X + W;
    Y2 = Y + H;
}

widget::~widget(void)
{
}