
#ifndef __OBJECT_PARCEL_IPC_H__
#define __OBJECT_PARCEL_IPC_H__

#include "AXP/cplusplus/xplatform/include/stl/hashTable.h"
#include "AXP/cplusplus/xplatform/include/Parcelable.h"
#include "AXP/cplusplus/xplatform/include/Parcel.h"

namespace ObjectParcel
{
    typedef AXP::Sp<AXP::CObject>(*PCreate)();

    class ClassFactory : public AXP::CObject
    {
    public:

        PCreate mCreate;
    };

    AXP::Sp<AXP::CObject> GetClassRef(
        IN CONST AXP::PCWStr key);

    AXP::Boolean InsertMappingTable(
        IN CONST AXP::PCWStr key,
        IN PCreate value);

    template<typename T>
    AXP::ARESULT WriteObjectToParcel(
        IN CONST AXP::Sp<AXP::CParcel> & parcel,
        IN CONST AXP::Sp<T> & value)
    {
        if (parcel == NULL)
            return AXP::AE_FAIL;

        AXP::Boolean hasValue = TRUE;
        if (value == NULL) {
            hasValue = FALSE;
            return parcel->WriteBoolean(hasValue);
        }

        if (AXP::AFAILED(parcel->WriteBoolean(hasValue)))
            return AXP::AE_FAIL;

        return value->WriteToParcel(parcel);
    }

    template<typename T>
    AXP::ARESULT WriteListOfObjectToParcel(
        IN CONST AXP::Sp<AXP::CParcel> & parcel,
        IN CONST AXP::Sp<AXP::List<T> > & value)
    {
        if (parcel == NULL)
            return AXP::AE_FAIL;

        AXP::Char type = 'L';
        if (AXP::AFAILED(parcel->Write((AXP::PByte)&type, sizeof(type))))
            return AXP::AE_FAIL;

        AXP::Int32 length;
        if (value == NULL)
            length = 0;
        else
            length = value->GetCount();        

        if (AXP::AFAILED(parcel->Write((AXP::PByte)&length, sizeof(length))))
            return AXP::AE_FAIL;

        if (value) {
            Foreach(T, obj, value) {
                if (obj == NULL)
                    return AXP::AE_FAIL;

                if (AXP::AFAILED(WriteObjectToParcel(parcel, obj)))
                    return AXP::AE_FAIL;
            }
        }

        return AXP::AS_OK;
    }

    template<typename T>
    AXP::ARESULT ReadObjectFromParcel(
        IN CONST AXP::Sp<AXP::CParcel> & parcel,
        OUT AXP::Sp<T> & value)
    {
        if (parcel == NULL)
            return AXP::AE_FAIL;

        AXP::Boolean hasValue;
        if (AXP::AFAILED(parcel->ReadBoolean(hasValue)))
            return AXP::AE_FAIL;

        if (hasValue) {
            AXP::Sp<AXP::String> className;
            AXP::Int64 pos = parcel->GetPosition();
            if (AXP::AFAILED(parcel->ReadString(className)))
                return AXP::AE_FAIL;

            parcel->Seek(pos);
            AXP::Sp<T> temp = GetClassRef((AXP::PCWStr)className->GetPayload());
            if (temp == NULL)
                return AXP::AE_FAIL;

            if (AXP::AFAILED(temp->ReadFromParcel(parcel)))
                return AXP::AE_FAIL;

            value = temp;
        }
        else {
            value = NULL;
        }

        return AXP::AS_OK;
    }

    template<typename T>
    AXP::ARESULT ReadListOfObjectFromParcel(
        IN CONST AXP::Sp<AXP::CParcel> & parcel,
        OUT AXP::Sp<AXP::List<T> > & value)
    {
        if (parcel == NULL)
            return AXP::AE_FAIL;

        AXP::Char type;
        if (AXP::AFAILED(parcel->Read((AXP::PByte)&type, sizeof(type), sizeof(type))))
            return AXP::AE_FAIL;

        if (type != 'L')
            return AXP::AE_FAIL;

        AXP::Int32 count;
        if (AXP::AFAILED(parcel->Read((AXP::PByte)&count, sizeof(count), sizeof(count))))
            return AXP::AE_FAIL;

        AXP::Sp<AXP::List<T> > list = new AXP::List<T>();
        if (list == NULL)
            return AXP::AE_OUTOFMEMORY;

        for (AXP::Int32 i = 0; i < count; i++) {
            AXP::Sp<T> obj;
            if (AXP::AFAILED(ReadObjectFromParcel(parcel, obj)))
                return AXP::AE_FAIL;

            if (obj == NULL) {
                value = NULL;
                return AXP::AE_FAIL;
            }

            if (!list->PushBack(obj))
                return AXP::AE_OUTOFMEMORY;
        }

        value = list;

        return AXP::AS_OK;
    }
}

#endif // __OBJECT_PARCEL_IPC_H__
