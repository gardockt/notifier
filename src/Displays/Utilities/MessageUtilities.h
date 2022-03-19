#ifndef MESSAGE_UTILITIES_H
#define MESSAGE_UTILITIES_H

#include <stdlib.h>

#include "../../Message.h"

Message* message_clone(const Message* message);
void message_free_all_children(Message* message);

#endif /* ifndef MESSAGE_UTILITIES_H */
