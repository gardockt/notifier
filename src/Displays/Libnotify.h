#ifndef LIBNOTIFY_H
#define LIBNOTIFY_H

#ifndef DISABLE_DISPLAY_LIBNOTIFY

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <libnotify/notify.h>

#include "Display.h"

bool libnotifyInit();
void libnotifyUninit();

bool libnotifyDisplayMessage(Message* message, void (*freeFunction)(Message*));

bool libnotifyStructure(Display* display);

#endif // ifndef DISABLE_DISPLAY_LIBNOTIFY

#endif // ifndef LIBNOTIFY_H
