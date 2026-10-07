#include "clouddestination.hpp"

namespace synqueen::cloud {

CloudDestination::CloudDestination(IPatchStorage &patchStorage)
    : patchStorage(patchStorage) {}

void CloudDestination::synchronize() {}

} // namespace synqueen::cloud
