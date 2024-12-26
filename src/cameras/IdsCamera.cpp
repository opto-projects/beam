#include "IdsCamera.h"

//#ifdef WITH_IDS

#include "cameras/CameraWorker.h"
#include "cameras/IdsCameraConfig.h"
#include "cameras/IdsHardConfig.h"
#include "opencv2/opencv.hpp"
#include <opencv2/core.hpp>

#include "helpers/OriDialogs.h"

#include <QSettings>
#include <QString>
#include <QVector>

#define LOG_ID "IdsComfortCamera:"
#define FRAME_TIMEOUT 5000
//#define LOG_FRAME_TIME

enum CamDataRow { ROW_RENDER_TIME, ROW_CALC_TIME,
    ROW_FRAME_ERR, ROW_FRAME_UNDERRUN, ROW_FRAME_DROPPED, ROW_FRAME_INCOMPLETE,
    ROW_BRIGHTNESS };

static QString makeDisplayName(const QString name)
{
    return QString("%1").arg(
        name);
}

static QString makeCustomId(int id)
{
    return QString("UVC-%2").arg(
        QString::number(id));
}

//------------------------------------------------------------------------------
//                              PeakIntf
//------------------------------------------------------------------------------

bool propertyExists(cv::VideoCapture* hCam, int propId)
{
    double get = hCam->get(propId);
    return hCam->set(propId, get);
}

class PeakIntf : public CameraWorker
{
public:
    int id;
    IdsCamera *cam;
    cv::VideoCapture hCam;
    cv::Mat frame;
    QByteArray hdrBuf;

    int framesErr = 0;
    int framesDropped = 0;
    int framesUnderrun = 0;
    int framesIncomplete = 0;

    int m_imgChannels; /*!< number of channels of the camera image due to current parameterization */
    int m_imgCols; /*!< cols of the camera image due to current parameterization */
    int m_imgRows; /*!< rows of the camera image due to current parameterization */
    int m_imgBpp; /*!< number of element size of the camera image due to current parameterization */
    cv::Mat m_pDataMatBuffer;
    
    enum tColorMode
    {
        modeAuto,
        modeColor,
        modeGray
    };

    PeakIntf(int id, PlotIntf *plot, TableIntf *table, IdsCamera *cam)
        : CameraWorker(plot, table, cam, cam, LOG_ID), id(id), cam(cam)
    {
        subtract = false;
        tableData = [cam, this]{
            QMap<int, CamTableData> data = {
                { ROW_RENDER_TIME, {avgAcqTime} },
                { ROW_CALC_TIME, {avgCalcTime} },
                { ROW_FRAME_ERR, {framesErr, CamTableData::COUNT, framesErr > 0} },
                { ROW_FRAME_DROPPED, {framesDropped, CamTableData::COUNT, framesDropped > 0} },
                { ROW_FRAME_UNDERRUN, {framesUnderrun, CamTableData::COUNT, framesUnderrun > 0} },
                { ROW_FRAME_INCOMPLETE, {framesIncomplete, CamTableData::COUNT, framesIncomplete > 0} },
            };
            if (cam->_cfg->showBrightness)
                data[ROW_BRIGHTNESS] = {brightness, CamTableData::VALUE3};
            return data;
        };
    }

    QString initResolution()
    {
        

        cam->_cfg->binning.configurable = false;
        cam->_cfg->decimation.configurable = false;

        int width =hCam.get(cv::CAP_PROP_FRAME_WIDTH);
        int height= hCam.get(cv::CAP_PROP_FRAME_HEIGHT);

        if (1) {
            propertyExists(&hCam,cv::CAP_PROP_FRAME_WIDTH);
            propertyExists(&hCam, cv::CAP_PROP_FRAME_HEIGHT);
        }


        // Reset ROI to max before settings binning/decimation
        cv::Rect  roiMin, roiMax, roiInc;
        roiMin.x = width / 2;
        roiMin.y = height / 2;
        roiMin.width = 320;
        roiMin.height = 240;
        roiMax = roiMin;
        roiMax.width = 1280;
        roiMax.height = 720;
        roiInc = roiMin;
        roiInc.width = 1;
        roiInc.height = 1;
        qDebug() << LOG_ID << "ROI"
            << QString("min=%1x%24").arg(roiMin.width).arg(roiMin.height)
            << QString("max=%1x%24").arg(roiMax.width).arg(roiMax.height)
            << QString("inc=%1x%24").arg(roiInc.width).arg(roiInc.height);

        cv::Rect roi;
        roi.x = width / 2;
        roi.y = height / 2;
        roi.width = width;
        roi.height = height;

        #define CLAMP_FACTOR(factor, list) \
            if (cam->_cfg->factor < 1) cam->_cfg->factor = 1; \
            else if (!cam->_cfg->list.contains(cam->_cfg->factor)) { \
                qWarning() << LOG_ID << #factor << cam->_cfg->factor << "is out of range, reset to 1"; \
                cam->_cfg->factor = 1; \
           }

        c.w = roi.width;
        c.h = roi.height;
        cam->_width = c.w;
        cam->_height = c.h;

        return {};
    }

    QString initPixelFormat()
    {
        cv::Mat pDataMatBuffer, grayDataMatBuffer;
        if (!hCam.retrieve(pDataMatBuffer))
        {

        }

        cv::cvtColor(pDataMatBuffer, grayDataMatBuffer, cv::COLOR_BGR2GRAY, 0);

        m_imgChannels = grayDataMatBuffer.channels();
        m_imgCols = grayDataMatBuffer.cols;
        m_imgRows = grayDataMatBuffer.rows;
        m_imgBpp = (int)grayDataMatBuffer.elemSize1() * 8;

        size_t formatCount = 1;
        QVector<int> pixelFormats(formatCount);
        pixelFormats[0] = m_imgBpp;
        QMap<int, int> supportedFormats;
        for (int i = 0; i < formatCount; i++) {
            auto pf = pixelFormats.at(i);
            if (pf == 8) {
                cam->_cfg->supportedBpp << 8;
                supportedFormats.insert(8, pf);
            } else if (pf == 10) {
                cam->_cfg->supportedBpp << 10;
                supportedFormats.insert(10, pf);
            } else if (pf == 12) {
                cam->_cfg->supportedBpp << 12;
                supportedFormats.insert(12, pf);
            }
        }
        if (supportedFormats.empty()) {
            return "Camera doesn't support any of known gray scale formats (Mono8, Mono10g40, Mono12g24)";
        }
        c.bpp = cam->_cfg->bpp;
        int targetFormat;
        if (!supportedFormats.contains(c.bpp)) {
            c.bpp = supportedFormats.firstKey();
            targetFormat = supportedFormats.first();
            qWarning() << LOG_ID << "Camera does not support " << cam->_cfg->bpp << "bpp, use " << c.bpp << "bpp";
        } else {
            targetFormat = supportedFormats[c.bpp];
        }
        qDebug() << LOG_ID << "Set pixel format" << QString::number(targetFormat, 16) << c.bpp << "bpp";

        cam->_cfg->bpp = c.bpp;
        if (c.bpp > 8) {
            hdrBuf = QByteArray(c.w*c.h*2, 0);
            c.buf = (uint8_t*)hdrBuf.data();
        }
        return {};
    }

    QString showCurrProps()
    {
        #define SHOW_CAM_PROP(prop, func, typ) {\
            typ val; \
            auto res = func(&hCam, val); \
            if ((res)) \
                qDebug() << LOG_ID << "Unable to get" << prop ; \
            else qDebug() << LOG_ID << prop << val; \
        }
        SHOW_CAM_PROP("FPS", getCamFps, double);
        SHOW_CAM_PROP("Exposure", getCamExp, double);
        #undef SHOW_CAM_PROP
        return {};
    }
    bool getCamExp(cv::VideoCapture* hCam, double& v) {
        return propertyExists(hCam, cv::CAP_PROP_EXPOSURE);
    }
    bool getCamFps(cv::VideoCapture* hCam, double& v) {
        return propertyExists(hCam, cv::CAP_PROP_FPS);
    }
    QString init()
    {
        cam->_name = makeDisplayName("webcam");
        cam->_customId = makeCustomId(0);
        cam->_descr = QString("%1 (SN: %2)").arg(
            QString::fromLatin1("webcam"),
            QString::fromLatin1("0"));
        cam->_configGroup = cam->_customId;
        cam->loadConfig();

        bool ret = hCam.open(id, cv::CAP_ANY);

        qDebug() << LOG_ID << "Camera opened" << id;

        if (auto err = initResolution(); !err.isEmpty()) return err;
        if (auto err = initPixelFormat(); !err.isEmpty()) return err;
        if (auto err = showCurrProps(); !err.isEmpty()) return err;

        plot->initGraph(c.w, c.h);
        graph = plot->rawGraph();

        configure();

        

        showBrightness = cam->_cfg->showBrightness;
        saveBrightness = cam->_cfg->saveBrightness;

        return {};
    }

    ~PeakIntf()
    {
        hCam.release();
    }

    void run()
    {
        qDebug() << LOG_ID << "Started" << QThread::currentThreadId();
        start = QDateTime::currentDateTime();
        timer.start();
        while (true) {
            tm = timer.elapsed();
            avgFrameCount++;
            avgFrameTime += tm - prevFrame;
            prevFrame = tm;

            tm = timer.elapsed();
            cv::Mat org;
            bool res = hCam.read(org);

            markAcqTime();


            if (res && (!org.empty())) {
                tm = timer.elapsed();
                cv::Mat orgClone = org.clone();
                cv::cvtColor(orgClone, frame, cv::COLOR_BGR2GRAY, 0);


                if (c.bpp == 12)
                    ;//cgn_convert_12g24_to_u16(c.buf, frame.data, frame.);
                else if (c.bpp == 10)
                    ;//cgn_convert_10g40_to_u16(c.buf, frame.data, buf.memorySize);
                else
                    c.buf = frame.data;


                calcResult();
                markCalcTime();

                if (showResults())
                    emit cam->ready();

            } else {
                framesErr++;
                stats[QStringLiteral("frameErrors")] = framesErr;
                QString errKey = QStringLiteral("frameError_") + QString::number(res, 16);
                stats[errKey] = stats[errKey].toInt() + 1;
            }

            if (tm - prevStat >= STAT_DELAY_MS) {
                prevStat = tm;


                double hardFps;
                
                hardFps = hCam.get(cv::CAP_PROP_FPS);

                double ft = avgFrameTime / avgFrameCount;
                avgFrameTime = 0;
                avgFrameCount = 0;
                CameraStats st {
                    .fps = 1000.0/ft,
                    .hardFps = hardFps,
                    .measureTime = measureStart > 0 ? timer.elapsed() - measureStart : -1,
                };
                emit cam->stats(st);

#ifdef LOG_FRAME_TIME
                qDebug()
                    << "FPS:" << st.fps
                    << "avgFrameTime:" << qRound(ft)
                    << "avgAcqTime:" << qRound(avgAcqTime)
                    << "avgCalcTime:" << qRound(avgCalcTime)
                    << "errCount: " << errCount
                    << IDS.getPeakError(res);
#endif
                if (cam->isInterruptionRequested()) {
                    qDebug() << LOG_ID << "Interrupted by user";
                    return;
                }
                checkReconfig();
            }
        }
    }
};

//------------------------------------------------------------------------------
//                              IdsCamera
//------------------------------------------------------------------------------

QVector<CameraItem> IdsCamera::getCameras()
{
    size_t camCount=1;
    QVector<int> cams(camCount);
    cams.append(0);

    QVector<CameraItem> result;
    for (const auto &cam : cams) {
        result << CameraItem {
            .cameraId = cam,
            .customId = makeCustomId(cam),
            .displayName = makeDisplayName("webcam"),
        };
    }
    return result;
}

void IdsCamera::unloadLib()
{
}

IdsCamera::IdsCamera(QVariant id, PlotIntf *plot, TableIntf *table, QObject *parent) :
    Camera(plot, table, "IdsComfortCamera"), QThread(parent)
{
    _cfg.reset(new IdsCameraConfig);

    auto peak = new PeakIntf(id.toInt(), plot, table, this);
    auto res = peak->init();
    if (!res.isEmpty())
    {
        Ori::Dlg::error(res);
        delete peak;
        return;
    }
    _peak.reset(peak);

    connect(parent, SIGNAL(camConfigChanged()), this, SLOT(camConfigChanged()));
}

IdsCamera::~IdsCamera()
{
    qDebug() << LOG_ID << "Deleted";
}

int IdsCamera::bpp() const
{
    return _cfg->bpp;
}

PixelScale IdsCamera::sensorScale() const
{
    return _pixelScale;
}

QList<QPair<int, QString>> IdsCamera::dataRows() const
{
    QList<QPair<int, QString>> rows =
    {
        { ROW_RENDER_TIME,      qApp->tr("Acq. time") },
        { ROW_CALC_TIME,        qApp->tr("Calc time") },
        { ROW_FRAME_ERR,        qApp->tr("Errors") },
        { ROW_FRAME_DROPPED,    qApp->tr("Dropped") },
        { ROW_FRAME_UNDERRUN,   qApp->tr("Underrun") },
        { ROW_FRAME_INCOMPLETE, qApp->tr("Incomplete") },
    };
    if (_cfg->showBrightness)
        rows << QPair<int, QString>{ ROW_BRIGHTNESS, qApp->tr("Brightness") };
    return rows;
}

QList<QPair<int, QString>> IdsCamera::measurCols() const
{
    QList<QPair<int, QString>> cols;
    if (_cfg->saveBrightness)
        cols << QPair<int, QString>{ COL_BRIGHTNESS, qApp->tr("Brightness") };
    return cols;
}

void IdsCamera::startCapture()
{
    start();
}

void IdsCamera::stopCapture()
{
    if (_peak)
        _peak.reset(nullptr);
}

void IdsCamera::startMeasure(MeasureSaver *saver)
{
    if (_peak)
        _peak->startMeasure(saver);
}

void IdsCamera::stopMeasure()
{
    if (_peak)
        _peak->stopMeasure();
}

void IdsCamera::run()
{
    if (_peak)
        _peak->run();
}

void IdsCamera::camConfigChanged()
{
    if (_peak)
        _peak->reconfigure();
}

void IdsCamera::saveHardConfig(QSettings* s)
{
    if (!_peak)
        return;
    double v;
    v = _peak->hCam.get(cv::CAP_PROP_EXPOSURE);

    s->setValue("exposure", v);
    v = _peak->hCam.get(cv::CAP_PROP_FPS);
    s->setValue("frameRate", v);
}

void IdsCamera::requestRawImg(QObject *sender)
{
    if (_peak)
        _peak->requestRawImg(sender);
}

void IdsCamera::setRawView(bool on, bool reconfig)
{
    if (_peak)
        _peak->setRawView(on, reconfig);
}

void IdsCamera::initConfigMore(Ori::Dlg::ConfigDlgOpts &opts)
{
    if (_peak)
        _cfg->initDlg(&_peak->hCam, opts, cfgMax);
}

void IdsCamera::saveConfigMore(QSettings *s)
{
    _cfg->save(s);
}

void IdsCamera::loadConfigMore(QSettings *s)
{
    _cfg->load(s);
}

HardConfigPanel* IdsCamera::hardConfgPanel(QWidget *parent)
{
    if (!_peak)
        return nullptr;
    if (!_configPanel) {
        auto requestBrightness = [this](QObject *s){ _peak->requestBrightness(s); };
        auto getCamProp = [this](IdsHardConfigPanel::CamProp prop) -> QVariant {
            switch (prop) {
            case IdsHardConfigPanel::AUTOEXP_FRAMES_AVG:
                return _cfg->autoExpFramesAvg;
            }
            return {};
        };
        _configPanel = new IdsHardConfigPanel(&_peak->hCam, requestBrightness, getCamProp, parent);
    }
    return _configPanel;
}

//#endif // WITH_IDS
