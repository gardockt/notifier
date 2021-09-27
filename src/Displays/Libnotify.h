#ifndef LIBNOTIFY_H
#define LIBNOTIFY_H

#ifdef ENABLE_DISPLAY_LIBNOTIFY

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <libnotify/notify.h>

#include "Display.h"

bool libnotifyInit();
void libnotifyUninit();

bool libnotifyDisplayMessage(Message* message);

bool libnotifyStructure(Display* display);

#endif // ifdef ENABLE_DISPLAY_LIBNOTIFY

#endif // ifndef LIBNOTIFY_H
