#ifndef PANEL_CALIBRATE_H
#define PANEL_CALIBRATE_H

#include <QWidget>
#include <QMap>
#include <QWizard>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QGraphicsScene>

#define WAIT -1

#define CALTYPE_ACCEL 0
#define CALTYPE_MAG 1
#define CALTYPE_XMIT 2

#define ACCEL_WAIT -1
#define ACCEL_RIGHTSIDEUP 0
#define ACCEL_UPSIDEDOWN 1
#define ACCEL_LEFT 2
#define ACCEL_RIGHT 3
#define ACCEL_UP 4
#define ACCEL_DOWN 5
#define ACCEL_FINISH 6

#define MAG_WAIT 7
#define MAG_ACQUIRE 8
#define MAG_FINISH 9

namespace Ui {
class PanelCalibrate;
}

class PanelCalibrate : public QWidget
{
    Q_OBJECT

public:
    explicit PanelCalibrate(QWidget *parent = 0);
    ~PanelCalibrate();
    bool connectState;
    QMap<QString, QString> configuration;
    void sendMessage(QString message) {emit messageOut(message.toUtf8());}

public slots:
    void initialize(QMap<QString, QString> config);
    void updateConnectionState(bool state) {connectState = state;}
    void parseMessage(QByteArray);

signals:
    void initializePanel(QMap<QString, QString>);
    void messageIn(QByteArray);
    void messageOut(QByteArray);
    void panelStatus(QString);
    void connectionState(bool);
    void getConnectionState();

private slots:
    void on_accelCal_clicked();
    void on_cancel_clicked();
    void on_next_clicked();
    void on_initEEPROM_clicked();
    void on_magCal_clicked();

    void on_xmitCal_clicked();

    void on_escCal_clicked();

    void on_horizontalSlider_valueChanged(int value);

private:
    Ui::PanelCalibrate *ui;
    int nextMessage;
    int calibrationType;
    QVector<float> accelX;
    QVector<float> accelY;
    QVector<float> accelZ;
    QVector<float> workingAccelX;
    QVector<float> workingAccelY;
    QVector<float> workingAccelZ;
    float minMagX;
    float maxMagX;
    float minMagY;
    float maxMagY;
    float minMagZ;
    float maxMagZ;
    bool storeAccelData(QString incomingData);
    float calculateAccelScaleFactor(float input1, float input2);
    QGraphicsScene *leftScene;
    QGraphicsScene *rightScene;
    QGraphicsEllipseItem *leftStick;
    QGraphicsEllipseItem *rightStick;
};

#endif // PANEL_CALIBRATE_H