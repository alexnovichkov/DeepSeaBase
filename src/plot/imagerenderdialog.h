#ifndef IMAGERENDERDIALOG_H
#define IMAGERENDERDIALOG_H

#include <QDialog>

class QLineEdit;
class QComboBox;
class FancyLineEdit;

class ImageRenderDialog : public QDialog
{
    Q_OBJECT
public:
    ImageRenderDialog(QWidget *parent = 0);
    inline QString getPath() const {return _path;};
    inline QSize getSize() const {return {_width, _height};}
    int getResolution() const;
    static QSize defaultSize() {return {257,145};}
    static int defaultResolution() {return 150;}
private:
    FancyLineEdit *pathEdit;
    QLineEdit *widthEdit;
    QLineEdit *heightEdit;
    QComboBox *resolutionCombo;

    QString _path;
    int _width = defaultSize().width();
    int _height = defaultSize().height();
    int _resolution = defaultResolution();
};

#endif // IMAGERENDERDIALOG_H
