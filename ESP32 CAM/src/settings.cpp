#include "settings.h"

classSettings Settings;

SortOrder classSettings::getSortOrder() {
  return currentOrder;
}

void classSettings::setSortOrder(SortOrder order) {
  currentOrder = order;
}
