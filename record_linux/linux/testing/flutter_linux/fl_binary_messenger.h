// testing/flutter_linux/fl_binary_messenger.h
#ifndef FL_BINARY_MESSENGER_H
#define FL_BINARY_MESSENGER_H

#include <glib-object.h>

G_BEGIN_DECLS

struct _FlBinaryMessenger;
typedef struct _FlBinaryMessenger FlBinaryMessenger;

#define FL_TYPE_BINARY_MESSENGER (fl_binary_messenger_get_type())
#define FL_BINARY_MESSENGER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FL_TYPE_BINARY_MESSENGER, FlBinaryMessenger))

GType fl_binary_messenger_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif // FL_BINARY_MESSENGER_H