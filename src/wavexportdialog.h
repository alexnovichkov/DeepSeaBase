#ifndef WAVEXPORTDIALOG_H
#define WAVEXPORTDIALOG_H

#include <QDialog>

class FileDescriptor;
class QProgressBar;
class QSpinBox;
class QLabel;
class TaskBarProgress;
class WavExporter;
class QThread;
class QDialogButtonBox;
class QComboBox;

class WavExportDialog : public QDialog
{
    Q_OBJECT
public:
    WavExportDialog(FileDescriptor *file, const QVector<int> &indexes, QWidget *parent=0);
    ~WavExportDialog();
    void accept() override;
    void reject() override;
private slots:
    void start();
    void stop();
    void updateProgressIndicator();
    void updateMaxProgress(int);
private:
    FileDescriptor *file;
    QVector<int> indexes;
    QSpinBox *channelsCount = nullptr;
    QProgressBar *bar = nullptr;
    QLabel *hintLabel = nullptr;
    TaskBarProgress *taskBarProgress = nullptr;
    WavExporter *exporter = nullptr;
    QThread *thread = nullptr;
    QDialogButtonBox *buttonBox;
    QComboBox *formatComboBox;
};

#endif // WAVEXPORTDIALOG_H
