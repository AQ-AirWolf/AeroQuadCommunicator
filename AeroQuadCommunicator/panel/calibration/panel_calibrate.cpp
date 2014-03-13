#include "panel_calibrate.h"
#include "ui_panel_calibrate.h"
#include <QDebug>
#include <QLabel>
#include <QMessageBox>

PanelCalibrate::PanelCalibrate(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PanelCalibrate)
{
    ui->setupUi(this);
    connect(this, SIGNAL(initializePanel(QMap<QString,QString>)), this, SLOT(initialize(QMap<QString,QString>)));
    connect(this, SIGNAL(messageIn(QByteArray)), this, SLOT(parseMessage(QByteArray)));
    connect(this, SIGNAL(connectionState(bool)), this, SLOT(updateConnectionState(bool)));

    ui->userConfirm->hide();
    ui->calPanel->setCurrentIndex(0);
    ui->displayInstructions->setText("Select a calibration to perform.");
    ui->next->setEnabled(false);
    ui->cancel->setEnabled(false);
    ui->calProgress->setValue(0);
}

PanelCalibrate::~PanelCalibrate()
{
    delete ui;
}

void PanelCalibrate::initialize(QMap<QString, QString> config)
{
    configuration = config;
    emit getConnectionState();
}

void PanelCalibrate::parseMessage(QByteArray data)
{
    QString incomingMessage = data;
    switch (nextMessage)
    {
    case ACCEL_WAIT:
        // Wait until user hits NEXT button to start calibration
        break;
    case ACCEL_RIGHTSIDEUP:
        if (storeAccelData(incomingMessage))
            nextMessage = ACCEL_RIGHTSIDEUP;
        else
            ui->accelInstructions->setText("Place the AeroQuad upside down on a flat surface.");
        break;
    case ACCEL_UPSIDEDOWN:
        if (storeAccelData(incomingMessage))
            nextMessage = ACCEL_UPSIDEDOWN;
        else
            ui->accelInstructions->setText("Point the left side down.");
        break;
    case ACCEL_LEFT:
        if (storeAccelData(incomingMessage))
            nextMessage = ACCEL_LEFT;
        else
            ui->accelInstructions->setText("Point the right side down.");
        break;
    case ACCEL_RIGHT:
        if (storeAccelData(incomingMessage))
            nextMessage = ACCEL_RIGHT;
        else
            ui->accelInstructions->setText("Point the nose up.");
        break;
    case ACCEL_UP:
        if (storeAccelData(incomingMessage))
            nextMessage = ACCEL_UP;
        else
            ui->accelInstructions->setText("Point the nose down.");
        break;
    case ACCEL_DOWN:
        if (storeAccelData(incomingMessage))
            nextMessage = ACCEL_DOWN;
        else
            ui->accelInstructions->setText("Return the AeroQuad to a flat surface.");
        break;
    case ACCEL_FINISH:
    {
        ui->calPanel->setCurrentIndex(0);
        ui->displayInstructions->setText("The accelerometer calibration is finished!");
        nextMessage = ACCEL_WAIT;
        float xAccelScaleFactor = calculateAccelScaleFactor(accelX.at(5), accelX.at(4));
        float yAccelScaleFactor = calculateAccelScaleFactor(accelY.at(3), accelY.at(2));
        float zAccelScaleFactor = calculateAccelScaleFactor(accelZ.at(0), accelZ.at(1));
        sendMessage("J");
        sendMessage("K" + QString::number(xAccelScaleFactor) + ";0;" + QString::number(yAccelScaleFactor) + ";0;" + QString::number(zAccelScaleFactor) + ";0;");
        sendMessage("W");
        ui->userConfirm->hide();

        break;
    }
    case MAG_WAIT:
        // Wait until user hits NEXT button to start calibraiton
        break;
    case MAG_ACQUIRE:
    {
        QStringList parseData = incomingMessage.split(',');
        minMagX = qMin(minMagX, parseData.at(0).toFloat());
        maxMagX = qMax(maxMagX, parseData.at(0).toFloat());
        minMagY = qMin(minMagY, parseData.at(1).toFloat());
        maxMagY = qMax(maxMagY, parseData.at(1).toFloat());
        minMagZ = qMin(minMagZ, parseData.at(2).toFloat());
        maxMagZ = qMax(maxMagZ, parseData.at(2).toFloat());
        ui->xAxisMinus->setValue(abs(minMagX));
        ui->xAxisPlus->setValue(maxMagX);
        ui->yAxisMinus->setValue(abs(minMagY));
        ui->yAxisPlus->setValue(maxMagY);
        ui->zAxisMinus->setValue(abs(minMagZ));
        ui->zAxisPlus->setValue(maxMagZ);
        ui->xAxisValue->setText(parseData.at(0));
        ui->yAxisValue->setText(parseData.at(1));
        ui->zAxisValue->setText(parseData.at(2).trimmed());
        break;
    }
    case MAG_FINISH:
    {
        QString magBiasX = QString::number((minMagX + maxMagX) / 2.0);
        QString magBiasY = QString::number((minMagY + maxMagY) / 2.0);
        QString magBiasZ = QString::number((minMagZ + maxMagZ) / 2.0);
        sendMessage("M" + magBiasX +";" + magBiasY + ";" + magBiasZ + ";");
        //qDebug() << "M" + magBiasX +";" + magBiasY + ";" + magBiasZ + ";";
        ui->calPanel->setCurrentIndex(0);
        ui->displayInstructions->setText("The magnetometer calibration is finished!");
        ui->userConfirm->hide();
        break;
    }
    }
}

void PanelCalibrate::on_accelCal_clicked()
{
    ui->calPanel->setCurrentIndex(1);
    ui->userConfirm->show();
    accelX.clear();
    accelY.clear();
    accelZ.clear();
    ui->accelInstructions->setText("Place the AeroQuad on a flat surface.");
    calibrationType = CALTYPE_ACCEL;
    sendMessage("l");
    nextMessage = ACCEL_WAIT;
    ui->next->setEnabled(true);
    ui->cancel->setEnabled(true);
    ui->calProgress->show();
}

float PanelCalibrate::calculateAccelScaleFactor(float input1, float input2)
{
    float m = (input2 - input1) / 19.613;
    float accelBias = input2 - (m * 9.0865);
    float biased = input2 - accelBias;
    return 9.8065 / biased;
}

void PanelCalibrate::on_cancel_clicked()
{
    ui->userConfirm->hide();
    ui->calPanel->setCurrentIndex(0);
    ui->displayInstructions->setText("Calibration has been cancelled.");
    sendMessage("X");
    nextMessage = WAIT;
}

void PanelCalibrate::on_next_clicked()
{
    switch(calibrationType)
    {
    case CALTYPE_ACCEL:
        nextMessage++;
        ui->next->setEnabled(false);
        sendMessage("l");
        break;
    case CALTYPE_MAG:
        nextMessage =  MAG_FINISH;
        break;
    case CALTYPE_XMIT:
        break;
    }
}

bool PanelCalibrate::storeAccelData(QString incomingMessage)
{
    QStringList parseData = incomingMessage.split(',');
    workingAccelX.append(parseData.at(0).toFloat());
    workingAccelY.append(parseData.at(1).toFloat());
    workingAccelZ.append(parseData.at(2).toFloat());
    int count = workingAccelX.size();
    ui->calProgress->setValue(count*2);
    if (count > 50)
    {
        float sumX = 0;
        float sumY = 0;
        float sumZ = 0;
        for (int index=0; index<count; index++)
        {
            sumX += workingAccelX.at(index);
            sumY += workingAccelY.at(index);
            sumZ += workingAccelZ.at(index);
        }
        accelX.append(sumX/(float)workingAccelX.size());
        accelY.append(sumY/(float)workingAccelY.size());
        accelZ.append(sumZ/(float)workingAccelZ.size());
        workingAccelX.clear();
        workingAccelY.clear();
        workingAccelZ.clear();
        ui->next->setEnabled(true);
        sendMessage("X");
        return false;
    }
    return true;
}

void PanelCalibrate::on_initEEPROM_clicked()
{
    ui->calPanel->setCurrentIndex(0);
    ui->displayInstructions->setText("");
    int response = QMessageBox::critical(this, "AeroQuad Communicator", "Are you sure you wish to initialize the EEPROM?  You will need to perform all board calibrations after this operation.", QMessageBox::Yes | QMessageBox::No);
    if (response == QMessageBox::Yes)
    {
        sendMessage("I");
        sendMessage("W");
        ui->displayInstructions->setText("EEPROM has been initialized.");
    }
    else
        ui->displayInstructions->setText("EEPROM has not been modified.");
}

void PanelCalibrate::on_magCal_clicked()
{
    ui->calPanel->setCurrentIndex(2);
    ui->userConfirm->show();
    ui->magInstructions->setText("Rotate the AeroQuad to determine the max/min value for each axis of the magnetometer.");
    calibrationType = CALTYPE_MAG;
    sendMessage("j");
    nextMessage = MAG_ACQUIRE;
    ui->next->setEnabled(true);
    ui->cancel->setEnabled(true);
    ui->calProgress->hide();
    minMagX = 1000000;
    maxMagX = -1000000;
    minMagY = 1000000;
    maxMagY = -1000000;
    minMagZ = 1000000;
    maxMagZ = -1000000;
    ui->xAxisMinus->setValue(0);
    ui->xAxisPlus->setValue(0);
    ui->yAxisMinus->setValue(0);
    ui->yAxisPlus->setValue(0);
    ui->zAxisMinus->setValue(0);
    ui->zAxisPlus->setValue(0);
    ui->xAxisValue->setText("0");
    ui->yAxisValue->setText("0");
    ui->zAxisValue->setText("0");
}
