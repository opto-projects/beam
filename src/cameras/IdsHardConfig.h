#ifndef IDS_HARD_CONFIG_H
#define IDS_HARD_CONFIG_H

//#ifdef WITH_IDS


#include "cameras/HardConfigPanel.h"
namespace cv {
    class VideoCapture;
}

class IdsHardConfigPanel : public HardConfigPanel
{
public:
    enum CamProp { AUTOEXP_FRAMES_AVG };

    IdsHardConfigPanel(cv::VideoCapture * hCam,
        std::function<void(QObject*)> requestBrightness,
        std::function<QVariant(CamProp)> getCamProp,
        QWidget *parent);

    void setReadOnly(bool on) override;

private:
    class IdsHardConfigPanelImpl *_impl;
};

//#endif // WITH_IDS
#endif // IDS_HARD_CONFIG_H
