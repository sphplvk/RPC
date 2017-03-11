
#include "IPC/cplusplus/lib/include/ObjectParcel.h"

using namespace ObjectParcel;

STATIC AXP::HashTable<AXP::PCWStr, AXP::CObject> * sMappingTable;
    
AXP::Boolean ObjectParcel::InsertMappingTable(IN CONST AXP::PCWStr key, IN PCreate value)
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
    
AXP::Sp<AXP::CObject> ObjectParcel::GetClassRef(IN CONST AXP::PCWStr key)
{
    AXP::Sp<ClassFactory> obj = sMappingTable->GetValue(key);
        
    if (obj == NULL)
        return NULL;
        
    return obj->mCreate();
}