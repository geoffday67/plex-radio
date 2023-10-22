#pragma once

enum SortOrder {
  Artist,
  Title
};

class classSettings {
public:
  SortOrder getSortOrder();
};

extern classSettings Settings;