#include "cGroupItem.h"
#include "mainwindow.h"
#include "ecsEvent.h"

cGroupItem::cGroupItem( NumberedItemModel* m, int i ) {
	cGroupModel = m;
	itemIndex = i;
	setAcceptDrops(true);
	setFlag( QGraphicsItem::ItemIsSelectable, true );
	maxChildWidth = 60;
	qDebug() << "Create " << i;
	addApplianceTexts();
}

void cGroupItem::recalcBoundingRect()
{
	int noLinks = cGroupModel->numberedItems[itemIndex].links.count();
	qreal penWidth = qApp->property( "cGroupPen" ).value<QPen>().width();
	float boxWidth = maxChildWidth + 10;

	float rw = boxWidth + penWidth;
	float rh = 10 + penWidth + (noLinks*lineSpacing);
	float rx = -rw / 2;
	float ry = -rh / 2;
	rect.setRect( rx, ry, rw, rh );

	qDebug() << "   boundingRect " << itemIndex << ": " << rect.x() << "," << rect.y() << "  " << rect.width() << "," << rect.height();
}

//------------------------------------------------------------------------------------

void cGroupItem::addApplianceTexts() {
	for( int i=0; i<cGroupModel->numberedItems[itemIndex].links.count(); i++ ) {
		QString app = QString::number(cGroupModel->numberedItems[itemIndex].links[i]->id)
					  + " - " + cGroupModel->numberedItems[itemIndex].links[i]->description;
		if( childItems().count() > i ) {
			qDebug() << "   Reusing appTextItem " + QString::number(i);
			((QGraphicsSimpleTextItem*)(childItems()[i]))->setText(app);
		}
		else {
			qDebug() << "   Creating appTextItem " + QString::number(i);
			QGraphicsSimpleTextItem* txtItem = new QGraphicsSimpleTextItem(app);
			txtItem->setParentItem(this);
			txtItem->setData( 0, this->itemIndex ); // data[0] is my parents cGroupItem index.
			txtItem->setData( 1, i );               // data[1] is my child index.
			txtItem->setZValue(2);
			txtItem->setFont( qApp->property( "contentFont" ).value<QFont>() );
			txtItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
		}
	}

	QList<QGraphicsItem*> ci = childItems();

	maxChildWidth = 60;
	for( int i=0; i<childItems().count(); i++ ) {
		float w = ((QGraphicsSimpleTextItem*)(childItems()[i]))->boundingRect().width();
		if( w > maxChildWidth ) { maxChildWidth = w; }
	}

	maxChildWidth += 30; // For source icon.

	recalcBoundingRect();

	for( int i=0; i<childItems().count(); i++ ) {
		((QGraphicsSimpleTextItem*)(childItems()[i]))->setPos( rect.x() + 6, rect.y() + 4 + lineSpacing*i );
	}
}

//------------------------------------------------------------------------------------

QPoint cGroupItem::anchorIn() {
	return QPoint(
				rect.x()  + this->pos().x(),
				rect.y() + rect.height()/2 + this->pos().y()
		   );
}

//------------------------------------------------------------------------------------

QPoint cGroupItem::anchorOut() {
	return QPoint(
				rect.x() + rect.width() + this->pos().x(),
				rect.y() + rect.height()/2 + this->pos().y()
		   );
}

//------------------------------------------------------------------------------------

void cGroupItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
					   QWidget *widget)
{
	qDebug() << "Paint " << QString::number(itemIndex) << " : " << this->isSelected();

	if( isSelected() ) {
		painter->setPen( Qt::NoPen );
		painter->setBrush( QColor( 0, 50, 255, 60 ) );
		painter->drawRect( rect.x()-5, rect.y()-25, rect.width()+10, rect.height()+30 );
	}

	painter->setBrush( qApp->property( "cGroupBrush" ).value<QBrush>() );
	painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
	painter->setFont( qApp->property( "headerFont" ).value<QFont>() );

	painter->drawRoundedRect(rect.x(), rect.y(), rect.width(), rect.height(), 6, 6);
	painter->drawText( rect.x(), rect.y()-6, QString::number(cGroupModel->numberedItems[itemIndex].id)
					   + " - " + cGroupModel->numberedItems[itemIndex].description );

	QPixmap* icon = new QPixmap(":/graphics/button.svg");

	float buttonDim = 17;

	for( int i=0; i<childItems().count(); i++ ) {
		QPointF pos = ((QGraphicsSimpleTextItem*)(childItems()[i]))->pos();
		pos.setX( pos.x() + ((QGraphicsSimpleTextItem*)(childItems()[i]))->boundingRect().width() + 10 );

		if( cGroupModel->numberedItems[itemIndex].ctrlFunctions.count() <= i ) continue;

		int func = cGroupModel->numberedItems[itemIndex].ctrlFunctions[i];
		if( func != ecsEvent::Unknown ) {
			painter->setFont( qApp->property( "buttonFont" ).value<QFont>() );

			switch( func ) {
			case ecsEvent::Key0: {
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *icon );
					painter->drawText( pos.x(), pos.y(), buttonDim, buttonDim, Qt::AlignHCenter|Qt::AlignVCenter, "1", 0 );
					break; }
			case ecsEvent::Key1: {
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *icon );
					painter->drawText( pos.x(), pos.y(), buttonDim, buttonDim, Qt::AlignHCenter|Qt::AlignVCenter, "2", 0 );
					break; }
			case ecsEvent::Key2: {
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *icon );
					painter->drawText( pos.x(), pos.y(), buttonDim, buttonDim, Qt::AlignHCenter|Qt::AlignVCenter, "3", 0 );
					break; }
			case ecsEvent::AnalogSignal: {
					QPixmap* sym = new QPixmap(":/graphics/signal.svg");
					painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *sym );
					break; }
			case ecsEvent::ChangeNotifiation: {
					QPixmap* sym = new QPixmap(":/graphics/connections.svg");
					painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *sym );
					break; }
			}
		}
	}
}

void cGroupItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
	event->setAccepted(event->mimeData()->hasFormat("x-application/ecs-appliance-id"));
}

void cGroupItem::dropEvent(QGraphicsSceneDragDropEvent *event)
{
	const QMimeData* data = event->mimeData();

	if( ! data->hasFormat("x-application/ecs-appliance-id") ) return;

	QString idString(data->data("x-application/ecs-appliance-id"));
	int applianceId = idString.toInt() ;
	NumberedItemModel* apps = ((MainWindow*)(qApp->activeWindow()))->applianceModel;
	NumberedItem* app = apps->findItem( applianceId );

	this->prepareGeometryChange();
	cGroupModel->numberedItems[itemIndex].links.append(app);
	cGroupModel->numberedItems[itemIndex].ctrlFunctions.append(ecsEvent::Key0);
	addApplianceTexts();
	recalcBoundingRect();
	((MainWindow*)qApp->activeWindow())->updateScene();
}
