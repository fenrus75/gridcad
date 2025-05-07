#include "gridcad.h"

port::port(void)
{
}

port::~port()
{
}

void port::add_wire(class wire* wire)
{
    wires.push_back(wire);
}

void port::update_value(struct value *newvalue)
{
    if (memcmp(&value, newvalue, sizeof(struct value)) == 0)
        return;
    for (auto wire : wires) {
        wire->update_value(newvalue);
    }
}