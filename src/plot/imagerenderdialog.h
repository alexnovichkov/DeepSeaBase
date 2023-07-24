#ifndef IMAGERENDERDIALOG_H
#define IMAGERENDERDIALOG_H

#include <QDialog>

class QLineEdit;
class QComboBox;
class FancyLineEdit;
class QCheckBox;

class ImageRenderDialog : public QDialog
{
    Q_OBJECT
public:
    struct PlotRenderOptions
    {
        QString path;
        QSize size;
        int resolution;
        bool graphOnly;
        int legendPosition;
    };
    ImageRenderDialog(bool askForPath, bool askForGraphOnly, QWidget *parent);
    ~ImageRenderDialog();
    PlotRenderOptions getRenderOptions() const;
    static PlotRenderOptions defaultRenderOptions();
//    inline QString getPath() const {return _path;};
//    inline QSize getSize() const {return {_width, _height};}
//    int getResolution() const;
    static QSize defaultSize() {return {257,145};}
    static int defaultResolution() {return 150;}
    inline void setAskForPath(bool ask) {askForPath = ask;}
//    bool graphOnly() const;
//    int getLegendPosition() const;
private:
    int getResolution(int index) const;
    FancyLineEdit *pathEdit = nullptr;
    QLineEdit *widthEdit = nullptr;
    QLineEdit *heightEdit = nullptr;
    QComboBox *resolutionCombo = nullptr;
    QCheckBox *graphOnlyCheckBox = nullptr;
    QComboBox *legendCombo = nullptr;

    QString _path;
    int _width = defaultSize().width();
    int _height = defaultSize().height();
    int _resolution = defaultResolution();
    bool askForPath = true;
};

#endif // IMAGERENDERDIALOG_H
