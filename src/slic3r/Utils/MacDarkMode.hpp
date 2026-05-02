#ifndef slic3r_MacDarkMode_hpp_
#define slic3r_MacDarkMode_hpp_

#include <wx/event.h>

namespace Slic3r {
namespace GUI {

#if __APPLE__
extern bool mac_dark_mode();
extern double mac_max_scaling_factor();
extern void set_miniaturizable(void * window);
void WKWebView_evaluateJavaScript(void * web, wxString const & script, void (*callback)(wxString const &));
void WKWebView_setTransparentBackground(void * web);
// SnapOrka: force a native NSView's appearance to Light (Aqua) regardless of system theme.
// Used for tab panels hosting WKWebView (Home, Device) — macOS 26 has a WKWebView
// dark-mode compositor regression that renders Flutter content as solid black.
void force_light_appearance(void * nsView);
void set_tag_when_enter_full_screen(bool isfullscreen);
void set_title_colour_after_set_title(void * window);
void initGestures(void * view,  wxEvtHandler * handler);
void openFolderForFile(wxString const & file);
#endif


} // namespace GUI
} // namespace Slic3r

#endif // MacDarkMode_h
