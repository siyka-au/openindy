#ifndef MOVESENSORDIALOG_H
#define MOVESENSORDIALOG_H

#include <QDialog>
#include <QShowEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "types.h"
#include "reading.h"
#include "modelmanager.h"

using namespace oi;

namespace Ui {
class MoveSensorDialog;
}

/*!
 * \brief The MovementDialog class
 * Dialog to specify a reading the active station's sensor shall be moved to
 */
class MoveSensorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MoveSensorDialog(QWidget *parent = 0);
    ~MoveSensorDialog();

signals:

    //################################
    //move the active station's sensor
    //################################

    void moveSensor(const Reading &reading);

private slots:

    //#########################
    //actions triggered by user
    //#########################

    //move sensor
    void on_pushButton_move_clicked();

    //reading type changed
    void on_comboBox_typeOfReading_currentTextChanged(const QString &arg1);

private:

    //##################################
    //methods to initialize GUI elements
    //##################################

    void initGUI();
    void initModels();
    void showEvent(QShowEvent *event);

private:

    //#################
    //helper attributes
    //#################

    //regular expressions to validate user input
    QRegularExpression validDouble;
    QRegularExpressionValidator *doubleValidator;

    Ui::MoveSensorDialog *ui;

};

#endif // MOVESENSORDIALOG_H
