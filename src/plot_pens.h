/* plot_pens.h — the pens (colours and widths) for the plotted curves.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * Each curve gets a pen from one list, shared by the main window and the plot
 * tabs. There are 50 default pens: 16 hues around the colour wheel, getting
 * darker for each further 16. A style sheet can then change the colour of any
 * of them (plotLineColor0 ... plotLineColor49) and the width of all of them
 * (plotLineWidth); see MainWindow::ReadStyle in mainwindow.cpp.
 *
 * Why this is a separate file: uTmax 3.01 built the default pens only while
 * reading the style sheet. If the style sheet named in cal.txt could not be
 * opened, the list stayed empty and the first plotted point divided by its
 * length (PlotTabWidget::plotUpdate, "gpnum % penList->length()"): a
 * floating-point exception on the first point of every sweep. The plot code also
 * read past the end of the list when the curve number jumped by more than one, or
 * when "colour by group" chose a pen by step number. Now the defaults are made
 * when the window is created, and every pen is fetched through penAt().
 */
#ifndef PLOT_PENS_H
#define PLOT_PENS_H

#include <QColor>
#include <QList>
#include <QPen>
#include <cmath>

constexpr int DEFAULT_PEN_COUNT = 50;
constexpr int PEN_HUES = 16;

// Default pen i: the same colours uTmax 3.01 made from the style sheet.
inline QPen defaultPen(int i)
{
    int h = 360 * (i % PEN_HUES) / PEN_HUES;
    int s = 200;
    int v = 230 - 100 * std::floor(i / PEN_HUES) / std::floor(DEFAULT_PEN_COUNT / PEN_HUES);
    QPen pen;
    pen.setColor(QColor::fromHsv(h, s, v, 255));
    pen.setWidthF(2);
    return pen;
}

// Set the first DEFAULT_PEN_COUNT pens to the defaults (any later ones are kept).
inline void setDefaultPens(QList<QPen> &pens)
{
    for (int i = 0; i < DEFAULT_PEN_COUNT; i++) {
        if (pens.length() <= i) pens.append(defaultPen(i));
        else pens.replace(i, defaultPen(i));
    }
}

// Pen number i, always valid. If the list is too short it is extended by
// repeating its pens from the start, as 3.01 did for one missing pen; an empty
// list first gets the defaults. A negative i gives pen 0.
inline QPen penAt(QList<QPen> &pens, int i)
{
    if (pens.isEmpty()) setDefaultPens(pens);
    if (i < 0) i = 0;
    int base = pens.length();
    while (pens.length() <= i) pens.append(pens.at(pens.length() % base));
    return pens.at(i);
}

#endif // PLOT_PENS_H
