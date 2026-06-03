## 核心接口

### `iwidget_tree.h`
```cpp
~IWidgetNode() = default;;
const std::string& id() const;;
WidgetType type() const;;
bool visible() const;;
bool enabled() const;;
Rect layoutBounds() const;;
void setLayoutBounds(const Rect& r);;
const std::vector<std::string>& childIds() const;;
JValue property(const std::string& key) const;;
void setProperty(const std::string& key, const JValue& val);;
bool hasProperty(const std::string& key) const;;
~IWidgetTree() = default;;
IWidgetNode* root();;
void setRoot(const std::string& id);;
IWidgetNode* get(const std::string& id);;
void add(std::unique_ptr<IWidgetNode> node);;
void remove(const std::string& id);;
void forEach(std::function<void(IWidgetNode&)>);;
size_t size() const;;
```

### `idata_model.h`
```cpp
~IDataModel() = default;;
void setValue(const std::string& path, const JValue& value);;
JValue getValue(const std::string& path) const;;
bool hasValue(const std::string& path) const;;
void clear();;
```

#### `WidgetType`
  - `Text, Button, TextField, CheckBox, Toggle`
  - `ChoicePicker, Slider, List, Grid, Tabs`
  - `Image, Divider, Row, Column, Card`
  - `Unknown`
## 渲染接口

### `irender_target.h`
```cpp
~IRenderTarget() = default;;
void beginDraw();;
void endDraw();;
void clear(float r, float g, float b);;
void drawText(const std::string& text, const Rect& rect,;
void drawRect(const Rect& rect, float r, float g, float b,;
void drawLine(float x1, float y1, float x2, float y2,;
Size measureText(const std::string& text, float fontSize,;
int width() const;;
int height() const;;
bool isValid() const;;
```

### `iwidget_state.h`
```cpp
~IWidgetState() = default;;
bool hovered() const;;
void setHovered(bool h);;
bool enabled() const;;
void setEnabled(bool e);;
void setOnChange(ChangeCallback cb);;
```
## 输入接口

### `iinput_dispatcher.h`
```cpp
~IInputDispatcher() = default;;
void dispatchMouseDown(int x, int y, int button);;
void dispatchMouseUp(int x, int y, int button);;
void dispatchMouseMove(int x, int y);;
void dispatchMouseWheel(float delta);;
void dispatchKeyDown(int vk);;
void dispatchKeyUp(int vk);;
void dispatchChar(uint32_t ch);;
```
## 布局接口

### `ilayout_engine.h`
```cpp
~ILayoutEngine() = default;;
void layout(core::IWidgetTree& tree, const Size& container);;
Size measure(core::IWidgetNode& node, const Size& constraint);;
```
## 事件总线

### `ievent_bus.h`
```cpp
~IEventBus() = default;;
void subscribe(EventType type, Handler handler);;
void publish(const Event& event);;
void publish(EventType type, const std::string& surfaceId = "",;
```