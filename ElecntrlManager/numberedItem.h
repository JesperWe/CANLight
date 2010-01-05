#ifndef NUMBEREDITEM_H
#define NUMBEREDITEM_H

#include <QString>
#include <QGraphicsItem>
#include "ecsEvent.h"

class NumberedItem
{

public:
    NumberedItem();
    static bool compareIdsAsc( const NumberedItem &a1, const NumberedItem &a2 );
    static bool compareIdsDesc( const NumberedItem &a1, const NumberedItem &a2 );
    static bool compareDscrAsc( const NumberedItem &a1, const NumberedItem &a2 );
    static bool compareDscrDesc( const NumberedItem &a1, const NumberedItem &a2 );

    int             id;
    QString         description;
    float           offset;

    QList<NumberedItem*> links;
    QList<int> events;
};

#endif // NUMBEREDITEM_H
