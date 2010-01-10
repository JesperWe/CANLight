#ifndef NUMBEREDITEM_H
#define NUMBEREDITEM_H

#include <QString>
#include <QGraphicsItem>

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
    QList<int> ctrlFunctions;
    QList<int> events;
    QList<int> actions;
    QList<int> targetGroupIndex;
};

#endif // NUMBEREDITEM_H
