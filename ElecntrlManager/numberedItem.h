#ifndef NUMBEREDITEM_H
#define NUMBEREDITEM_H

#include <QString>
#include <QGraphicsItem>

class NumberedItem
{

public:
    enum itemTypes_e {
        None,
        Controller,
        Effector,
        noItemTypes
    };

    NumberedItem();
    static bool compareIdsAsc( const NumberedItem &a1, const NumberedItem &a2 );
    static bool compareIdsDesc( const NumberedItem &a1, const NumberedItem &a2 );
    static bool compareDscrAsc( const NumberedItem &a1, const NumberedItem &a2 );
    static bool compareDscrDesc( const NumberedItem &a1, const NumberedItem &a2 );

    QVariant typeIcon() const;

    int             id;
    QString         description;
    int             itemType;
    float           offset;

    QList<NumberedItem*> links;
    QList<int> ctrlFunctions;
    QList<int> events;
    QList<int> actions;
    QList<int> targetGroups;
};

#endif // NUMBEREDITEM_H
