
#import "../include/ObjectParcel.h"
#include "AXP/cplusplus/xplatform/include/stl/hashTable.h"

STATIC AXP::HashTable<AXP::PCWStr, AXP::CObject> * sMappingTable;

STATIC AXP::Mutex sMutex;

class ClassFactory : public AXP::CObject
{
public:
    
    PCreate mCreate;
};

AXP::Boolean InsertMappingTable(IN CONST AXP::PCWStr key, IN PCreate value)
{
    if ((!key) || (!value))
        return FALSE;

    if (sMappingTable == NULL) {
        sMappingTable = new AXP::HashTable<AXP::PCWStr, AXP::CObject>(50);
        if (sMappingTable == NULL)
            return FALSE;
    }

    AXP::Sp<ClassFactory> obj = new ClassFactory();
    if (obj == NULL)
        return FALSE;

    obj->mCreate = value;

    return sMappingTable->InsertUnique(key, obj);
}

id GetClassRef(IN CONST AXP::PCWStr key)
{
    Synchronized(&sMutex) {
        AXP::Sp<ClassFactory> obj = sMappingTable->GetValue(key);
        if (!obj)
            return nil;

        return obj->mCreate();
    }
    
    return nil;
}

AXP::ARESULT WriteObjectToParcel(CParcel * parcel, id<IParcelable> value)
{
    if (parcel == nil)
        return AXP::AE_FAIL;
    
    AXP::Boolean hasValue = TRUE;
    if (value == NULL) {
        hasValue = FALSE;
        return [parcel WriteBoolean:hasValue];
    }
    
    if (AXP::AFAILED([parcel WriteBoolean:hasValue]))
        return AXP::AE_FAIL;
    
    return [value WriteToParcel:parcel];
}

AXP::ARESULT WriteListOfObjectToParcel(CParcel * parcel,  NSMutableArray * value)
{
    if (parcel == NULL)
        return AXP::AE_FAIL;
    
    if (value == NULL)
        return [parcel WriteInt32:0];
    
    if (AXP::AFAILED([parcel WriteInt32:(int32_t)value.count]))
        return AXP::AE_FAIL;
    
    for (id<IParcelable>  obj in value) {
        if (AXP::AFAILED(WriteObjectToParcel(parcel, obj)))
            return AXP::AE_FAIL;
    }
    
    return AXP::AS_OK;
}

id ReadObjectFromParcel(CParcel * parcel)
{
    if (parcel == NULL)
        @throw [NSException exceptionWithName: @"" reason: @"" userInfo: nil];
    
    Boolean hasValue;
    if (AXP::AFAILED([parcel ReadBoolean:&hasValue]))
        @throw [NSException exceptionWithName: @"" reason: @"" userInfo: nil];
   
    if (!hasValue)
        return nil;
    
    int64_t pos = [parcel GetPosition];
    
    NSString * className = [parcel ReadString];
    [parcel Seek:pos];
    AXP::Sp<AXP::String> className2 = AXP::String::Create((AXP::WChar*)[className cStringUsingEncoding:NSUTF32LittleEndianStringEncoding]);
    if (className2 == NULL)
        return nil;
    
    id<IParcelable> temp = GetClassRef((AXP::PCWStr)className2->GetPayload());
    if (temp == NULL)
        @throw [NSException exceptionWithName: @"" reason: @"" userInfo: nil];

    if (AXP::AFAILED([temp ReadFromParcel:parcel]))
        @throw [NSException exceptionWithName: @"" reason: @"" userInfo: nil];
    
    return temp;
}

NSMutableArray * ReadListOfObjectFromParcel(CParcel * parcel)
{
    if (parcel == nil)
        @throw [NSException exceptionWithName: @"" reason: @"" userInfo: nil];
    
    int32_t count;
    if (AXP::AFAILED([parcel ReadInt32:&count]))
        @throw [NSException exceptionWithName: @"" reason: @"" userInfo: nil];
    
    if (count == 0)
        return nil;
    
    NSMutableArray * array = [[NSMutableArray alloc] init];    
    if (array == NULL)
        @throw [NSException exceptionWithName: @"" reason: @"" userInfo: nil];
    
    for (int32_t i = 0; i < count; ++i) {
        id<IParcelable> obj = ReadObjectFromParcel(parcel);
        [array addObject: obj];
    }
    
    return array;
}