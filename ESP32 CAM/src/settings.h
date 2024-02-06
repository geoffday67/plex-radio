#pragma once

enum SortOrder {
  Artist,
  Title
};

class classSettings {
 private:
  SortOrder currentOrder;

 public:
  classSettings() : currentOrder(Title) {}

  SortOrder getSortOrder();
  void setSortOrder(SortOrder);
};

extern classSettings Settings;