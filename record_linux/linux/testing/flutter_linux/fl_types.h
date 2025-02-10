// testing/flutter_linux/fl_types.h
#ifndef FL_TYPES_H
#define FL_TYPES_H

#include <glib-object.h>

G_BEGIN_DECLS

// Autoptr declarations for Flutter types
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlMethodChannel, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlStandardMethodCodec, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlMethodResponse, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlBinaryMessenger, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlMethodCall, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlMethodCodec, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FlValue, g_object_unref)

G_END_DECLS

#endif // FL_TYPES_H