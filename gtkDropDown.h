
#ifndef GTKDROPDOWN_H
#define GTKDROPDOWN_H
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
bool selectWalkingSpeed(ezgl::application *application, double & walkingSpeed, double & walkingTime);
void storeTime(GtkWidget * widget, double * t);
void storeSpeed(GtkWidget * widget, double * s);



#endif /* GTKDROPDOWN_H */

