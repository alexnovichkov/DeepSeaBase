#ifndef FILTERHEADERVIEW_H
#define FILTERHEADERVIEW_H

#include <QHeaderView>
#include <QWidgetAction>

class QLineEdit;

//class FilterHeaderView : public QHeaderView
//{
//    Q_OBJECT
//public:
//    FilterHeaderView(Qt::Orientation orientation, QWidget *parent = Q_NULLPTR);
//    void setFilterBoxes(int count);
//    void clear();
//signals:
//    void filterChanged(const QString &text, int column);
//private slots:
//    void adjustPositions();
//private:
//    int _padding;
//    QList<QLineEdit*> _editors;
//public:
//    virtual QSize sizeHint() const override;
//protected slots:
//    virtual void updateGeometries() override;
//};

class EditMenuAction : public QWidgetAction {
    Q_OBJECT
public:
    EditMenuAction(int section, const QString &filter, const QString &sectionText, QObject *parent);
signals:
    void filterChanged(const QString &text, int column);
private slots:
    void onTextChanged(const QString & text);
protected:
    virtual QWidget *createWidget(QWidget *parent) override;
private:
    int section;
    QString filter;
    QString sectionText;
};

class FilteredHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    FilteredHeaderView(Qt::Orientation orientation, QWidget *parent = Q_NULLPTR);
    void clear();
signals:
    void filterChanged(const QString &text, int column);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
};

#endif // FILTERHEADERVIEW_H
