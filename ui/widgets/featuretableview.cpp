#include "featuretableview.h"

#include <QPainter>

FeatureTableView::FeatureTableView(QWidget *parent) : QTableView(parent),
    backgroundImage(":/Images/icons/openIndy_bg.png")
{
}

void FeatureTableView::paintEvent(QPaintEvent *event)
{
    if(!this->backgroundImage.isNull()){
        QPainter painter(this->viewport());
        QPoint center(
            (this->viewport()->width() - this->backgroundImage.width()) / 2,
            (this->viewport()->height() - this->backgroundImage.height()) / 2
        );
        painter.drawPixmap(center, this->backgroundImage);
    }

    QTableView::paintEvent(event);
}
