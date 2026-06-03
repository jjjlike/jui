#pragma once
#include <vector>
#include <string>

namespace jui {

enum class WidgetType {
    Text, Button, TextField, CheckBox, Toggle,
    ChoicePicker, Slider, List, Grid, Tabs,
    Image, Divider, Row, Column, Card,
    Unknown
};

struct Children {
    enum Mode { Explicit, None } mode = None;
    std::vector<std::string> explicitList;
};

} // namespace jui
