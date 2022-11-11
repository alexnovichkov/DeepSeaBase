#include "qcpitemrichtext.h"
#include "logging.h"

class QCPRichTextDocument : public QTextDocument
{
public:
    QCPRichTextDocument( const QString& text, int flags, const QFont& font )
    {
        setUndoRedoEnabled( false );
        setDefaultFont( font );
        setHtml( text );

        // make sure we have a document layout
        ( void )documentLayout();

        QTextOption option = defaultTextOption();
        if ( flags & Qt::TextWordWrap )
            option.setWrapMode( QTextOption::WordWrap );
        else
            option.setWrapMode( QTextOption::NoWrap );

        option.setAlignment( static_cast< Qt::Alignment >( flags ) );
        setDefaultTextOption( option );

        QTextFrame* root = rootFrame();
        QTextFrameFormat fm = root->frameFormat();
        fm.setBorder( 0 );
        fm.setMargin( 0 );
        fm.setPadding( 0 );
        fm.setBottomMargin( 0 );
        fm.setLeftMargin( 0 );
        root->setFrameFormat( fm );

        adjustSize();
    }
};

QCPItemRichText::QCPItemRichText(QCustomPlot *parentPlot) : QCPItemText(parentPlot)
{

}

bool QCPItemRichText::underMouse(const QPoint &pos, double *distanceX, double *distanceY)
{
    auto anchorPos = position->parentAnchor()->pixelPosition();


    QTransform transform;
    transform.translate(pos.x(), pos.y());
    if (!qFuzzyIsNull(mRotation))
      transform.rotate(mRotation);

    QCPRichTextDocument doc( mText, 0, mainFont() );
    QTextOption option = doc.defaultTextOption();
    option.setWrapMode( QTextOption::NoWrap );
    doc.setDefaultTextOption( option );
    doc.adjustSize();

    QRectF textRect;
    textRect.setSize(doc.size());
    QRectF textBoxRect = textRect.adjusted(-mPadding.left(), -mPadding.top(), mPadding.right(), mPadding.bottom());
    QPointF textPos = getTextDrawPoint(QPointF(0, 0), textBoxRect, mPositionAlignment); // 0, 0 because the transform does the translation
    textBoxRect.moveTopLeft(textPos + anchorPos);

    if (distanceX) *distanceX = qAbs(pos.x() - (textBoxRect.center().x()));
    if (distanceY) *distanceY = qAbs(pos.y() - (textBoxRect.center().y()));

    return textBoxRect.contains(pos);
}


void QCPItemRichText::draw(QCPPainter *painter)
{
    QPointF pos(position->pixelPosition());
    QTransform transform = painter->transform();
    transform.translate(pos.x(), pos.y());
    if (!qFuzzyIsNull(mRotation))
      transform.rotate(mRotation);
    painter->setFont(mainFont());

    QCPRichTextDocument doc( mText, 0, mainFont() );

    QTextOption option = doc.defaultTextOption();
    if ( option.wrapMode() != QTextOption::NoWrap )
    {
        option.setWrapMode( QTextOption::NoWrap );
        doc.setDefaultTextOption( option );
        doc.adjustSize();
    }

    QRectF textRect;
    textRect.setSize(doc.size());
    QRectF textBoxRect = textRect.adjusted(-mPadding.left(), -mPadding.top(), mPadding.right(), mPadding.bottom());



    QPointF textPos = getTextDrawPoint(QPointF(0, 0), textBoxRect, mPositionAlignment); // 0, 0 because the transform does the translation
    textRect.moveTopLeft(textPos.toPoint() + QPoint(mPadding.left(), mPadding.top()));
    textBoxRect.moveTopLeft(textPos.toPoint());
    int clipPad = qCeil(mainPen().widthF());

    QRectF boundingRect = textBoxRect.adjusted(-clipPad, -clipPad, clipPad, clipPad);
    if (transform.mapRect(boundingRect).intersects(painter->transform().mapRect(clipRect())))
    {
      painter->setTransform(transform);
      if ((mainBrush().style() != Qt::NoBrush && mainBrush().color().alpha() != 0) ||
          (mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0))
      {
        painter->setPen(mainPen());
        painter->setBrush(mainBrush());
        painter->drawRect(textBoxRect);
      }
      painter->setBrush(Qt::NoBrush);
      painter->setPen(QPen(mainColor()));

//      painter->drawText(textRect, Qt::TextDontClip|mTextAlignment, mText);
      QStaticText staticText(mText);
      painter->drawStaticText(textRect.topLeft(), staticText);
    }
}

QPointF QCPItemRichText::anchorPixelPosition(int anchorId) const
{
    QPointF pos(position->pixelPosition());
    QTransform transform;
    transform.translate(pos.x(), pos.y());
    if (!qFuzzyIsNull(mRotation))
      transform.rotate(mRotation);

    QCPRichTextDocument doc( mText, 0, mainFont() );
        QTextOption option = doc.defaultTextOption();
    if ( option.wrapMode() != QTextOption::NoWrap )
    {
        option.setWrapMode( QTextOption::NoWrap );
        doc.setDefaultTextOption( option );
        doc.adjustSize();
    }

    QRectF textRect;
    textRect.setSize(doc.size());
    QRectF textBoxRect = textRect.adjusted(-mPadding.left(), -mPadding.top(), mPadding.right(), mPadding.bottom());

    QPointF textPos = getTextDrawPoint(QPointF(0, 0), textBoxRect, mPositionAlignment); // 0, 0 because the transform does the translation
    textBoxRect.moveTopLeft(textPos.toPoint());

    QPolygonF rectPoly = transform.map(QPolygonF(textBoxRect));

    switch (anchorId)
    {
      case aiTopLeft:     return rectPoly.at(0);
      case aiTop:         return (rectPoly.at(0)+rectPoly.at(1))*0.5;
      case aiTopRight:    return rectPoly.at(1);
      case aiRight:       return (rectPoly.at(1)+rectPoly.at(2))*0.5;
      case aiBottomRight: return rectPoly.at(2);
      case aiBottom:      return (rectPoly.at(2)+rectPoly.at(3))*0.5;
      case aiBottomLeft:  return rectPoly.at(3);
      case aiLeft:        return (rectPoly.at(3)+rectPoly.at(0))*0.5;
    }

    LOG(DEBUG) << Q_FUNC_INFO << "invalid anchorId" << anchorId;
    return {};
}
