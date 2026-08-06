#ifndef FEATURETABLEVIEW_H
#define FEATURETABLEVIEW_H

#include <QTableView>
#include <QPaintEvent>

/*!
 * \brief The FeatureTableView class
 * A plain QTableView that paints a centered, non-repeating watermark
 * behind its content. Deliberately does not use a widget stylesheet to
 * achieve this - setting any stylesheet property on a QTableView routes
 * all of its item painting (hover, selection, ...) through Qt's CSS
 * emulation layer instead of the native QStyle, which some styles (e.g.
 * KDE's Breeze) render inconsistently for item views.
 */
class FeatureTableView : public QTableView
{
    Q_OBJECT

public:
    explicit FeatureTableView(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap backgroundImage;
};

#endif // FEATURETABLEVIEW_H
