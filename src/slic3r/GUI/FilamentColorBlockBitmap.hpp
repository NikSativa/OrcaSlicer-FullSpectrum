#pragma once

#include <wx/bitmap.h>
#include <wx/colour.h>
#include <wx/string.h>

#include <vector>

namespace Slic3r { namespace GUI {

struct CornerRadius
{
    int m_topLeft     = 0;
    int m_topRight    = 0;
    int m_bottomLeft  = 0;
    int m_bottomRight = 0;

    static CornerRadius Uniform(int r) {
        return {r, r, r, r};
    }
    bool IsZero() const {
        return m_topLeft == 0 && m_topRight == 0 && m_bottomLeft == 0 && m_bottomRight == 0;
    }
};

// Unified colour-block parameters used by both solid and gradient swatches.
struct ColorBlockParams
{
    enum Mode { Solid, Gradient };
    Mode mode = Solid;
    wxColour solid_color;
    std::vector<wxColour> gradient_colors; // 2-stop gradient, already sorted (bottom→top)
    wxString label;
    int width  = 20;
    int height = 20;
};

// Linear interpolation across an ordered list of colours (0.0 → colors[0], 1.0 → colors.back()).
wxColour interpolate_color(const std::vector<wxColour>& colors, double pos);

// Cached colour-block bitmap. The static BitmapCache lives inside the implementation.
// Key format:  "solid:#RRGGBB:hH:wW:label"  or  "grad:#RRGGBB:#RRGGBBBT:hH:wW:label"
wxBitmap* get_color_block_bitmap_cached(const ColorBlockParams& params);

// Cached bitmap for official filament colour blocks. Multiple colours are drawn left to right.
wxBitmap* get_color_block_bitmap_cached(const std::vector<wxColour>& colors, bool is_gradient,
                                        int width, int height, const wxString& label,
                                        const wxColour& lightBorderColor,
                                        const CornerRadius& radius = {});

}} // namespace Slic3r::GUI
