#ifndef OPENMVCAMERASETTINGS_H
#define OPENMVCAMERASETTINGS_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtNetwork>

#include <utils/hostosinfo.h>

namespace Ui
{
    class OpenMVCameraSettings;
}

namespace OpenMV {
namespace Internal {

class OpenMVCameraSettings : public QDialog
{
    Q_OBJECT

public:

    explicit OpenMVCameraSettings(const QString &fileName, QWidget *parent = Q_NULLPTR);
    ~OpenMVCameraSettings();

public slots:

    void accept();

private:

    QSettings *m_settings;
    Ui::OpenMVCameraSettings *m_ui;
};

} // namespace Internal
} // namespace OpenMV

#endif // OPENMVCAMERASETTINGS_H
