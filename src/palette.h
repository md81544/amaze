#pragma once

// The colours of the ship status panel (see View), which the menu dialog
// shares so the two look like parts of the same ship's systems

#include <QColor>
#include <QString>

namespace marengo {
namespace amaze {

// Labels, borders and the bars' outlines
inline const QColor STATUS_COLOUR(0, 128, 0);
// The filled part of the bars: darker than the outline, but brighter than the
// arena's background grid
inline const QColor BAR_FILL_COLOUR(0, 80, 0);
inline const QColor PANEL_BACKGROUND_COLOUR(0, 14, 0);
// The inner bezel line and the dividers between sections
inline const QColor PANEL_BEZEL_COLOUR(0, 56, 0);
inline const QColor PANEL_VALUE_COLOUR(0, 210, 0);
inline const QColor PANEL_FAULT_COLOUR(210, 40, 40);
inline const QColor PANEL_DEPLETED_COLOUR(220, 150, 0);

// For style sheets, e.g. "rgb(0, 128, 0)"
inline QString cssColour(const QColor& colour)
{
    return QString("rgb(%1, %2, %3)").arg(colour.red()).arg(colour.green()).arg(colour.blue());
}

} // namespace amaze
} // namespace marengo
