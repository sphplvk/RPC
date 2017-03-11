
#import "AXP/objective-c/Parcel.h"
#import "AXP/objective-c/IParcelable.h"
#include "AXP/cplusplus/xplatform/include/type.h"

typedef id (*PCreate)();

AXP::Boolean InsertMappingTable(
    IN CONST AXP::PCWStr key,
    IN PCreate value);

AXP::ARESULT WriteObjectToParcel(
    IN CParcel * parcel,
    IN id<IParcelable> value);

AXP::ARESULT WriteListOfObjectToParcel(
    IN CParcel * parcel,
    IN NSMutableArray * value);

id ReadObjectFromParcel(
    IN CParcel * parcel);

NSMutableArray * ReadListOfObjectFromParcel(
    IN CParcel * parcel);