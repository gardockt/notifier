#ifndef DUNST_H
#define DUNST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libnotify/notify.h>

#include "Display.h"

bool dunstInit();
void dunstUninit();

bool dunstDisplayMessage(Message* message);

bool dunstStructure(Display* display);

#endif // ifndef DUNST_H
