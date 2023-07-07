#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMap>

class QtTreePropertyBrowser;
class QtVariantPropertyManager;
class QtVariantEditorFactory;
class QtProperty;

class SettingsDialog : public  QDialog
{
Q_OBJECT
public:
    SettingsDialog(QWidget *parent = 0);
public Q_SLOTS:
    void accept();
Q_SIGNALS:
    void closed();
private Q_SLOTS:
    void propertyChanged(QtProperty*p, const QVariant&v);
private:
    void addSettings();

    QtTreePropertyBrowser *propertyTree;
    QtVariantPropertyManager *m_manager;
    QtVariantEditorFactory *m_factory;
    QMap<QString, QString> m_displayNames;
};


#endif // SETTINGSDIALOG_H
