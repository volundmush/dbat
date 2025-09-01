#include "dbat/HasExtraDescriptions.h"

const std::vector<ExtraDescription> &HasExtraDescriptions::getExtraDescription() const
{
    return extra_descriptions;
}