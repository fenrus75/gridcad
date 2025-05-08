#include "gridcad.h"

port::port(int _direction)
{
    direction = _direction;
}

port::~port()
{
}

void port::add_wire(class wire* wire)
{
    wires.push_back(wire);
    if (direction == PORT_OUT) {
        printf("OUT PURT update %i  \n", value.boolval);
        wire->update_value(&value);
    }
}

void port::update_value(struct value *newvalue)
{
    printf("Updating value of port to %i \n", newvalue->boolval);
    if (memcmp(&value, newvalue, sizeof(struct value)) == 0)
        return;
    for (auto wire : wires) {
        wire->update_value(newvalue);
    }
}