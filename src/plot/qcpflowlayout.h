#ifndef QCPFLOWLAYOUT_H
#define QCPFLOWLAYOUT_H

#include "qcustomplot.h"

//class QCPFlowLayout : public QCPLayout
//{
//    Q_OBJECT
//public:
//    QCPFlowLayout();

//    Qt::Orientation orientation() const {return m_orientation;}
//    void setOrientation(Qt::Orientation orientation);

//    int columnSpacing() const;
//    void setColumnSpacing(int columnSpacing);

//    int rowSpacing() const;
//    void setRowSpacing(int rowSpacing);

//    bool hasElement(QCPLayoutElement *element) const;

//    bool appendElement(QCPLayoutElement *element);
//    bool addElement(QCPLayoutElement *element);
//    bool prependElement(QCPLayoutElement *element);
//    bool insertElement(QCPLayoutElement *element, int index);

//    // QCPLayout interface
//public:
//    virtual int elementCount() const override;
//    virtual QCPLayoutElement *elementAt(int index) const override;
//    virtual QCPLayoutElement *takeAt(int index) override;
//    virtual bool take(QCPLayoutElement *element) override;

//    // QCPLayoutElement interface
//public:
//    virtual QSize minimumOuterSizeHint() const override;
//    virtual QSize maximumOuterSizeHint() const override;
//protected:
//    virtual void updateLayout() override;
//private:
//    QVector<QCPLayoutElement*> m_elements;
//    Qt::Orientation m_orientation = Qt::Horizontal;
//    int m_columnSpacing = 5;
//    int m_rowSpacing = 5;
//};

#endif // QCPFLOWLAYOUT_H
