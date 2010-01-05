#include "numberedItem.h"

NumberedItem::NumberedItem()
{
    id = 0;
    description = "N/A";
}

bool NumberedItem::compareIdsAsc( const NumberedItem &a1, const NumberedItem &a2 ) { return a1.id < a2.id; }

bool NumberedItem::compareIdsDesc( const NumberedItem &a1, const NumberedItem &a2 ) { return a1.id > a2.id; }

bool NumberedItem::compareDscrAsc( const NumberedItem &a1, const NumberedItem &a2 ) { return a1.description < a2.description; }

bool NumberedItem::compareDscrDesc( const NumberedItem &a1, const NumberedItem &a2 ) { return a1.description > a2.description; }
