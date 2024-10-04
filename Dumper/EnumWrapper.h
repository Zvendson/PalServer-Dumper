#pragma once
#include "EnumManager.h"

class EnumWrapper
{
private:
    const UEEnum Enum;
    EnumInfoHandle InfoHandle;

public:
    EnumWrapper(const UEEnum Enm);

public:
    UEEnum GetUnrealEnum() const;

public:
    std::string GetName() const;
    std::string GetRawName() const;
    std::string GetFullName() const;

    std::pair<std::string, bool> GetUniqueName() const;
    uint8 GetUnderlyingTypeSize() const;

    size_t GetNumMembers() const;

    CollisionInfoIterator GetMembers() const;

    bool IsValid() const;
};
