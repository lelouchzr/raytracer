#pragma once
#include "rt/engine.hpp"

namespace rt {
class StudioWidgets {
    static unsigned editedItem_;
    static void recordEdit(const char *key);

  public:
    static void configureInputs();
    static void beginEdits();
    static unsigned editedItem();
    static Json &member(Json &, const std::string &, Json fallback);
    static bool number(Json &, const char *, const char *, double, double, double,
                       bool integer = false);
    static bool vector(Json &, const char *, const char *, Vec3);
    static bool color(Json &, const char *, const char *, Vec3);
    static bool text(Json &, const char *, const char *, const std::string &fallback = "");
    static bool flag(Json &, const char *, const char *, bool = false);
    static bool combo(Json &, const char *, const char *, const std::vector<std::string> &,
                      const std::string &);
    static Json vec(Vec3);
};
} // namespace rt
