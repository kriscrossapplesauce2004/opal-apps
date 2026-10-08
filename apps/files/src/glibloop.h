#pragma once

// Qt and GLib main loop glue.
// Qt on Linux uses QEventDispatcherGlib by default, which iterates the default GMainContext.
// Async GIO callbacks therefore arrive on the GUI thread. checkDispatcher() verifies this at startup
// and installs a polling timer when Qt uses another dispatcher.
namespace GlibLoop {

// Returns true when the GLib dispatcher is active. Safe to call many times.
bool checkDispatcher();

// True when a fallback timer pumps the GLib default context.
bool usesFallbackPump();

}
