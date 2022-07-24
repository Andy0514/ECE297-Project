
#include "gtkDropDown.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include <string>

void storeSpeed(GtkWidget * widget, double * s){
    const char * result = gtk_entry_get_text ((GtkEntry*) widget);
    std::string res(result);
    
    try {
        std::stod(res);
    } catch (const std::invalid_argument & ia){
        return;
    }
    *s = std::stod(res);
}
void storeTime(GtkWidget * widget, double * t){
    const char * result = gtk_entry_get_text ((GtkEntry*) widget);
    std::string res(result);
    
    try {
        std::stod(res);
    } catch (const std::invalid_argument & ia){
        return;
    }
    *t = std::stod(res);
}
bool selectWalkingSpeed(ezgl::application *application, double & walkingSpeed, double & walkingTime)
{

    // BEGIN: CODE FOR SHOWING DIALOG
    GObject *window;
    GtkWidget *content_area;
    GtkWidget *label1, *label2;
    GtkWidget *dialog;
    GtkWidget * speed;
    GtkWidget * time;
    GtkWidget * separator;

    // get a pointer to the main application window
    window = application->get_object(application->get_main_window_id().c_str());

    dialog = gtk_dialog_new_with_buttons(
        "Enter walking speed",
        (GtkWindow*) window,
        GTK_DIALOG_MODAL,
        ("OK"),
        GTK_RESPONSE_ACCEPT,
        ("CANCEL"),
        GTK_RESPONSE_REJECT,
        NULL
    );
    // Create a label and attach it to the content area of the dialog
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    label1 = gtk_label_new("Enter walking speed (m/s):");
    label2 = gtk_label_new("Enter walking time limit (s):");
    separator = gtk_label_new("");
    speed = gtk_entry_new();
    time = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content_area), label1);
    gtk_container_add(GTK_CONTAINER(content_area), speed);
    gtk_container_add(GTK_CONTAINER(content_area), separator);
    gtk_container_add(GTK_CONTAINER(content_area), label2);
    gtk_container_add(GTK_CONTAINER(content_area), time);
    g_signal_connect(G_OBJECT(speed), "changed", G_CALLBACK(storeSpeed), &walkingSpeed);
    g_signal_connect(G_OBJECT(time), "changed", G_CALLBACK(storeTime), &walkingTime);
    // The main purpose of this is to show dialog’s child widget, label

    gtk_widget_show_all(dialog);
    int result = gtk_dialog_run ((GtkDialog *)dialog);
    gtk_widget_destroy(GTK_WIDGET (dialog));
    
    switch (result){
        case GTK_RESPONSE_ACCEPT:
            return true;
        case GTK_RESPONSE_DELETE_EVENT:
            walkingSpeed = walkingTime = -1;
            return false;
        case GTK_RESPONSE_REJECT:
            walkingSpeed = walkingTime = -1;
            return false;
        default:
            walkingSpeed = walkingTime = -1;
            return false;   
    }
}

