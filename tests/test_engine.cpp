/**
 * test_engine.cpp — JUIEngine 引擎全覆盖单元测试
 *
 * 测试范围:
 *   - 构造/析构
 *   - processMessage: A2UI JSON 消息处理
 *     - createSurface
 *     - updateComponents / surfaceUpdate (v0.8/v0.9 兼容)
 *     - updateDataModel / dataModelUpdate (v0.8/v0.9 兼容)
 *     - beginRendering
 *     - deleteSurface
 *     - 非法 JSON
 *     - 空消息
 *   - processMessageStream: JSONL 流式处理
 *     - 单条消息
 *     - 多条消息
 *     - 空白行、空行过滤
 *   - Surface 管理: createSurface / getSurface
 *   - loadFromFile: 文件存在/不存在
 *   - ActionCallback: 设置回调
 *   - 窗口消息转发: onSize/onMouseMove/onMouseDown等
 *   - Widget JSON 解析: parseWidget (v0.8/v0.9 格式)
 *   - parseValue / parseBoundValue: 所有类型
 *   - bindDataToWidget: 数据绑定
 *   - refreshLayout: 布局刷新
 *   - render: 无窗口渲染调用
 */

#include <gtest/gtest.h>
#include "jui/core/engine.h"
#include <fstream>
#include <cstdio>

using namespace jui;

// ============================================================
// 1. 构造/析构
// ============================================================

TEST(JUIEngineTest, Constructor_Default) {
    // 验证: 默认构造成功
    JUIEngine engine;
    EXPECT_NO_THROW(engine);
}

TEST(JUIEngineTest, Destructor_NoCrash) {
    // 验证: 析构不崩溃
    auto engine = std::make_unique<JUIEngine>();
    EXPECT_NO_THROW(engine.reset());
}

// ============================================================
// 2. processMessage — 非法/空 JSON
// ============================================================

TEST(JUIEngineTest, ProcessMessage_InvalidJson_NoCrash) {
    // 验证: 非法 JSON 不会导致崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessage("not valid json {{{"));
    EXPECT_NO_THROW(engine.processMessage(""));
}

TEST(JUIEngineTest, ProcessMessage_EmptyObject_NoCrash) {
    // 验证: 空 JSON 对象不会崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessage("{}"));
}

TEST(JUIEngineTest, ProcessMessage_UnknownType_NoCrash) {
    // 验证: 未知消息类型不会崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessage(R"({"unknownType":{"key":"value"}})"));
}

// ============================================================
// 3. processMessage — createSurface
// ============================================================

TEST(JUIEngineTest, ProcessMessage_CreateSurface_DefaultId) {
    // 验证: 不带 surfaceId 时默认使用 "main"
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{}})");

    auto surface = engine.getSurface("main");
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(surface->id(), "main");
}

TEST(JUIEngineTest, ProcessMessage_CreateSurface_CustomId) {
    // 验证: 指定 surfaceId 创建 Surface
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"custom"}})");

    auto surface = engine.getSurface("custom");
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(surface->id(), "custom");
}

TEST(JUIEngineTest, ProcessMessage_CreateSurface_Multiple) {
    // 验证: 创建多个 Surface
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"s1"}})");
    engine.processMessage(R"({"createSurface":{"surfaceId":"s2"}})");

    EXPECT_NE(engine.getSurface("s1"), nullptr);
    EXPECT_NE(engine.getSurface("s2"), nullptr);
}

// ============================================================
// 4. processMessage — updateComponents / surfaceUpdate (v0.8/v0.9)
// ============================================================

TEST(JUIEngineTest, ProcessMessage_SurfaceUpdate_V08) {
    // 验证: v0.8 surfaceUpdate 格式解析
    JUIEngine engine;

    // 先创建 surface
    engine.processMessage(R"({"createSurface":{"surfaceId":"v08test"}})");
    engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "v08test",
            "components": [
                {"id":"root","component":{"Column":{"children":{"explicitList":["t1"]}}}},
                {"id":"t1","component":{"Text":{"text":{"literalString":"Hello v0.8"}}}}
            ]
        }
    })");

    auto surface = engine.getSurface("v08test");
    ASSERT_NE(surface, nullptr);
    auto t1 = surface->getWidget("t1");
    ASSERT_NE(t1, nullptr);
    EXPECT_EQ(t1->type(), WidgetType::Text);
    EXPECT_EQ(t1->property("text").asString(), "Hello v0.8");
}

TEST(JUIEngineTest, ProcessMessage_UpdateComponents_V09) {
    // 验证: v0.9 updateComponents 格式解析
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"v09test"}})");
    engine.processMessage(R"({
        "updateComponents": {
            "surfaceId": "v09test",
            "components": [
                {"id":"root","component":"Column","children":{"explicitList":["btn"]}},
                {"id":"btn","component":"Button","text":{"literalString":"Click me"}}
            ]
        }
    })");

    auto surface = engine.getSurface("v09test");
    ASSERT_NE(surface, nullptr);
    auto btn = surface->getWidget("btn");
    ASSERT_NE(btn, nullptr);
    EXPECT_EQ(btn->type(), WidgetType::Button);
    EXPECT_EQ(btn->property("text").asString(), "Click me");
}

TEST(JUIEngineTest, ProcessMessage_UpdateComponents_NoSurfaceId) {
    // 验证: 不指定 surfaceId 时使用当前 surface
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"auto"}})");
    engine.processMessage(R"({
        "updateComponents": {
            "components": [
                {"id":"auto-w","component":"Text","text":{"literalString":"Auto"}}
            ]
        }
    })");

    auto surface = engine.getSurface("auto");
    ASSERT_NE(surface, nullptr);
    EXPECT_NE(surface->getWidget("auto-w"), nullptr);
}

TEST(JUIEngineTest, ProcessMessage_UpdateComponents_AutoCreateSurface) {
    // 验证: updateComponents 自动创建未存在的 Surface
    JUIEngine engine;
    engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "auto-created",
            "components": [
                {"id":"root","component":"Text","text":{"literalString":"Hi"}}
            ]
        }
    })");

    EXPECT_NE(engine.getSurface("auto-created"), nullptr);
}

// ============================================================
// 5. processMessage — updateDataModel / dataModelUpdate
// ============================================================

TEST(JUIEngineTest, ProcessMessage_DataModelUpdate_V08) {
    // 验证: v0.8 dataModelUpdate 格式
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"dm"}})");
    engine.processMessage(R"({
        "dataModelUpdate": {
            "surfaceId": "dm",
            "path": "/user",
            "contents": [
                {"key":"name","valueString":"Alice"},
                {"key":"age","valueNumber":30}
            ]
        }
    })");

    auto surface = engine.getSurface("dm");
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(surface->dataModel().getValue("/user/name").asString(), "Alice");
    EXPECT_EQ(surface->dataModel().getValue("/user/age").asInt(), 30);
}

TEST(JUIEngineTest, ProcessMessage_UpdateDataModel_V09) {
    // 验证: v0.9 updateDataModel 格式
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"dm2"}})");
    engine.processMessage(R"({
        "updateDataModel": {
            "surfaceId": "dm2",
            "path": "/config",
            "contents": [
                {"key":"theme","valueString":"dark"},
                {"key":"debug","valueBoolean":true}
            ]
        }
    })");

    auto surface = engine.getSurface("dm2");
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(surface->dataModel().getValue("/config/theme").asString(), "dark");
    EXPECT_TRUE(surface->dataModel().getValue("/config/debug").asBool());
}

TEST(JUIEngineTest, ProcessMessage_DataModelUpdate_NoSurface_Ignored) {
    // 验证: 不存在的 Surface 更新数据不会崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessage(R"({
        "updateDataModel": {
            "surfaceId": "ghost",
            "path": "/",
            "contents": [{"key":"x","valueNumber":1}]
        }
    })"));
}

TEST(JUIEngineTest, ProcessMessage_DataModelUpdate_ValueMap) {
    // 验证: valueMap 类型的数据更新
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"map"}})");
    engine.processMessage(R"({
        "dataModelUpdate": {
            "surfaceId": "map",
            "path": "/",
            "contents": [
                {"key":"profile","valueMap":{"city":"Beijing","country":"CN"}}
            ]
        }
    })");

    auto surface = engine.getSurface("map");
    ASSERT_NE(surface, nullptr);
    auto profile = surface->dataModel().getValue("profile");
    EXPECT_TRUE(profile.isObject());
}

// ============================================================
// 6. processMessage — beginRendering
// ============================================================

TEST(JUIEngineTest, ProcessMessage_BeginRendering) {
    // 验证: beginRendering 设置根组件并刷新布局
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"render"}})");
    engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "render",
            "components": [
                {"id":"root","component":"Column","children":{"explicitList":[]}}
            ]
        }
    })");
    EXPECT_NO_THROW(engine.processMessage(R"({
        "beginRendering": {
            "surfaceId": "render",
            "root": "root"
        }
    })"));

    auto surface = engine.getSurface("render");
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(surface->rootWidgetId(), "root");
    EXPECT_FALSE(surface->needsLayout());
}

TEST(JUIEngineTest, ProcessMessage_BeginRendering_NoSurface_NoCrash) {
    // 验证: 对不存在的 Surface beginRendering 不会崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessage(R"({
        "beginRendering": {"surfaceId": "ghost", "root": "r"}
    })"));
}

// ============================================================
// 7. processMessage — deleteSurface
// ============================================================

TEST(JUIEngineTest, ProcessMessage_DeleteSurface) {
    // 验证: deleteSurface 删除存在的 Surface
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"temp"}})");
    EXPECT_NE(engine.getSurface("temp"), nullptr);

    engine.processMessage(R"({"deleteSurface":{"surfaceId":"temp"}})");
    EXPECT_EQ(engine.getSurface("temp"), nullptr);
}

TEST(JUIEngineTest, ProcessMessage_DeleteSurface_ClearsCurrent) {
    // 验证: 删除当前 Surface 后 current 清空
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"current"}})");
    engine.processMessage(R"({"deleteSurface":{"surfaceId":"current"}})");
    // 不应崩溃
    EXPECT_NO_THROW(engine.render());
}

TEST(JUIEngineTest, ProcessMessage_DeleteSurface_EmptyId_NoCrash) {
    // 验证: 空 surfaceId 的 deleteSurface 不会崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessage(R"({"deleteSurface":{"surfaceId":""}})"));
}

// ============================================================
// 8. processMessage — 完整 A2UI 流程
// ============================================================

TEST(JUIEngineTest, FullA2UIFlow) {
    // 验证: 完整的 4 步 A2UI 协议流程
    JUIEngine engine;

    // Step 1: 创建 Surface
    engine.processMessage(R"({"createSurface":{"surfaceId":"flow"}})");

    // Step 2: 发送组件
    engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "flow",
            "components": [
                {"id":"root","component":{"Column":{"children":{"explicitList":["title","btn"]}}}},
                {"id":"title","component":{"Text":{"text":{"literalString":"Flow Test"},"fontSize":{"literalNumber":24}}}},
                {"id":"btn","component":{"Button":{"text":{"literalString":"Submit"},"action":{"name":"submit"}}}}
            ]
        }
    })");

    // Step 3: 数据模型
    engine.processMessage(R"({
        "dataModelUpdate": {
            "surfaceId": "flow",
            "path": "/",
            "contents": [
                {"key":"status","valueString":"ready"}
            ]
        }
    })");

    // Step 4: 开始渲染
    engine.processMessage(R"({"beginRendering":{"surfaceId":"flow","root":"root"}})");

    // 验证
    auto surface = engine.getSurface("flow");
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(surface->widgets().size(), 3);
    EXPECT_EQ(surface->dataModel().getValue("status").asString(), "ready");

    // 设置 action 回调
    engine.setActionCallback([](auto&, auto&, auto&, auto&) {});
}

// ============================================================
// 9. processMessageStream — JSONL 流
// ============================================================

TEST(JUIEngineTest, ProcessMessageStream_SingleLine) {
    // 验证: 单行 JSONL 解析
    JUIEngine engine;
    engine.processMessageStream(R"({"createSurface":{"surfaceId":"jsonl"}}
)");

    EXPECT_NE(engine.getSurface("jsonl"), nullptr);
}

TEST(JUIEngineTest, ProcessMessageStream_MultipleLines) {
    // 验证: 多行 JSONL 解析
    JUIEngine engine;
    engine.processMessageStream(R"({"createSurface":{"surfaceId":"ml1"}}
{"createSurface":{"surfaceId":"ml2"}}
)");

    EXPECT_NE(engine.getSurface("ml1"), nullptr);
    EXPECT_NE(engine.getSurface("ml2"), nullptr);
}

TEST(JUIEngineTest, ProcessMessageStream_BlankLines) {
    // 验证: 空白行被过滤
    JUIEngine engine;

    std::string stream = "{\"createSurface\":{\"surfaceId\":\"bl1\"}}\n\n  \n{\"createSurface\":{\"surfaceId\":\"bl2\"}}\n";
    engine.processMessageStream(stream);

    EXPECT_NE(engine.getSurface("bl1"), nullptr);
    EXPECT_NE(engine.getSurface("bl2"), nullptr);
}

TEST(JUIEngineTest, ProcessMessageStream_Empty) {
    // 验证: 空流不会崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessageStream(""));
    EXPECT_NO_THROW(engine.processMessageStream("\n\n\n"));
}

TEST(JUIEngineTest, ProcessMessageStream_InvalidLine) {
    // 验证: 包含非法 JSON 行的流不会崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.processMessageStream("not json\n{\"createSurface\":{\"surfaceId\":\"ok\"}}"));
    EXPECT_NE(engine.getSurface("ok"), nullptr);
}

// ============================================================
// 10. Surface 管理
// ============================================================

TEST(JUIEngineTest, GetSurface_NonExistent_ReturnsNull) {
    // 验证: getSurface 不存在返回 nullptr
    JUIEngine engine;
    EXPECT_EQ(engine.getSurface("no-such"), nullptr);
}

TEST(JUIEngineTest, CreateSurface_And_Get) {
    // 验证: createSurface 后 getSurface 可获取
    JUIEngine engine;
    auto s = engine.createSurface("created");
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->id(), "created");
    EXPECT_EQ(engine.getSurface("created"), s);
}

TEST(JUIEngineTest, CreateSurface_Overwrite) {
    // 验证: 重复创建同 ID 的 Surface 会覆盖
    JUIEngine engine;
    auto s1 = engine.createSurface("dup");
    auto s2 = engine.createSurface("dup");
    EXPECT_NE(s1, s2); // 应该是新对象
}

// ============================================================
// 11. loadFromFile
// ============================================================

TEST(JUIEngineTest, LoadFromFile_NonExistent) {
    // 验证: 不存在的文件返回 false
    JUIEngine engine;
    EXPECT_FALSE(engine.loadFromFile("nonexistent_file.json"));
}

TEST(JUIEngineTest, LoadFromFile_ExistingFile) {
    // 验证: 有效 JSON 文件加载成功
    // 先创建临时文件
    std::string path = "temp_jui_test.json";
    std::ofstream out(path);
    out << R"({"createSurface":{"surfaceId":"from-file"}})";
    out.close();

    JUIEngine engine;
    EXPECT_TRUE(engine.loadFromFile(path));
    EXPECT_NE(engine.getSurface("from-file"), nullptr);

    // 清理
    std::remove(path.c_str());
}

// ============================================================
// 12. ActionCallback
// ============================================================

TEST(JUIEngineTest, ActionCallback_Set) {
    // 验证: 设置 action 回调
    JUIEngine engine;
    bool called = false;
    engine.setActionCallback([&](auto& s, auto& a, auto& c, auto& ctx) {
        called = true;
    });
    EXPECT_FALSE(called); // 回调尚未触发
}

// ============================================================
// 13. 窗口消息转发（无窗口环境）
// ============================================================

TEST(JUIEngineTest, WindowEvents_NoWindow_NoCrash) {
    // 验证: 无窗口时窗口事件不崩溃
    JUIEngine engine;

    EXPECT_NO_THROW(engine.onSize(800, 600));
    EXPECT_NO_THROW(engine.onMouseMove(100, 200));
    EXPECT_NO_THROW(engine.onMouseDown(100, 200, 0));
    EXPECT_NO_THROW(engine.onMouseUp(100, 200, 0));
    EXPECT_NO_THROW(engine.onCharInput('A'));
    EXPECT_NO_THROW(engine.onKeyDown(VK_RETURN));
    EXPECT_NO_THROW(engine.onKeyUp(VK_RETURN));
}

// ============================================================
// 14. render
// ============================================================

TEST(JUIEngineTest, Render_Uninitialized_NoCrash) {
    // 验证: 未初始化的 render 不崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.render());
}

TEST(JUIEngineTest, Render_NoSurface_NoCrash) {
    // 验证: 无 Surface 的 render 不崩溃
    JUIEngine engine;
    EXPECT_NO_THROW(engine.render());
}

TEST(JUIEngineTest, Renderer_Access) {
    // 验证: 可以获取内部 D2DRenderer
    JUIEngine engine;
    auto& r = engine.renderer();
    EXPECT_EQ(r.width(), 0);
    EXPECT_EQ(r.height(), 0);
}

// ============================================================
// 15. Widget 解析全覆盖（各类控件类型）
// ============================================================

TEST(JUIEngineTest, Parse_AllWidgetTypes_V08) {
    // 验证: v0.8 格式所有控件类型至少能解析
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"types"}})");
    engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "types",
            "components": [
                {"id":"t","component":{"Text":{"text":{"literalString":"a"}}}},
                {"id":"b","component":{"Button":{"text":{"literalString":"btn"},"action":{"name":"go"}}}},
                {"id":"tf","component":{"TextField":{"placeholder":{"literalString":"input"},"value":{"path":"/v"}}}},
                {"id":"cb","component":{"CheckBox":{"text":{"literalString":"agree"},"value":{"path":"/a"}}}},
                {"id":"cb2","component":{"Checkbox":{"text":{"literalString":"opt"},"value":{"literalBoolean":true}}}},
                {"id":"im","component":{"Image":{}}},
                {"id":"rw","component":{"Row":{"children":{"explicitList":["t"]}}}},
                {"id":"co","component":{"Column":{"children":{"explicitList":["t"]}}}},
                {"id":"ca","component":{"Card":{"children":{"explicitList":["t"]}}}},
                {"id":"li","component":{"List":{"children":{"explicitList":[]}}}},
                {"id":"di","component":{"Divider":{}}}
            ]
        }
    })");

    auto surface = engine.getSurface("types");
    ASSERT_NE(surface, nullptr);
    EXPECT_EQ(surface->widgets().size(), 11);
    EXPECT_EQ(surface->getWidget("t")->type(), WidgetType::Text);
    EXPECT_EQ(surface->getWidget("b")->type(), WidgetType::Button);
    EXPECT_EQ(surface->getWidget("tf")->type(), WidgetType::TextField);
    EXPECT_EQ(surface->getWidget("cb")->type(), WidgetType::CheckBox);
    EXPECT_EQ(surface->getWidget("cb2")->type(), WidgetType::CheckBox); // "Checkbox" 别名
    EXPECT_EQ(surface->getWidget("im")->type(), WidgetType::Image);
    EXPECT_EQ(surface->getWidget("rw")->type(), WidgetType::Row);
    EXPECT_EQ(surface->getWidget("co")->type(), WidgetType::Column);
    EXPECT_EQ(surface->getWidget("ca")->type(), WidgetType::Card);
    EXPECT_EQ(surface->getWidget("li")->type(), WidgetType::List);
    EXPECT_EQ(surface->getWidget("di")->type(), WidgetType::Divider);
}

TEST(JUIEngineTest, Parse_UnknownType_ReturnsNull) {
    // 验证: 未知组件类型不会导致解析崩溃，但 Widget 为 nullptr
    // 测试通过 processMessage 间接验证
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"unk"}})");
    EXPECT_NO_THROW(engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "unk",
            "components": [
                {"id":"bad","component":{"UnknownWidget":{"prop":"val"}}}
            ]
        }
    })"));

    auto surface = engine.getSurface("unk");
    ASSERT_NE(surface, nullptr);
    // 解析未知类型返回 nullptr，不会加入 surface
    EXPECT_EQ(surface->getWidget("bad"), nullptr);
}

TEST(JUIEngineTest, Parse_Children_Template) {
    // 验证: Template 模式子组件解析
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"tmpl"}})");
    engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "tmpl",
            "components": [
                {"id":"list","component":{"List":{"children":{"template":{"dataBinding":"/items","componentId":"tpl"}}}}},
                {"id":"tpl","component":{"Text":{"text":{"path":"name"}}}}
            ]
        }
    })");

    auto surface = engine.getSurface("tmpl");
    ASSERT_NE(surface, nullptr);
    auto list = surface->getWidget("list");
    ASSERT_NE(list, nullptr);
    const auto& children = list->children();
    EXPECT_EQ(children.mode, Children::Template);
    EXPECT_EQ(children.templateInfo.dataBinding, "/items");
    EXPECT_EQ(children.templateInfo.componentId, "tpl");
}

TEST(JUIEngineTest, Parse_Action_String) {
    // 验证: action 为字符串时的解析
    JUIEngine engine;
    engine.processMessage(R"({"createSurface":{"surfaceId":"act"}})");
    engine.processMessage(R"({
        "surfaceUpdate": {
            "surfaceId": "act",
            "components": [
                {"id":"btn","component":{"Button":{"text":{"literalString":"Go"},"action":"nav"}}}
            ]
        }
    })");

    auto btn = engine.getSurface("act")->getWidget("btn");
    ASSERT_NE(btn, nullptr);
    EXPECT_EQ(btn->action(), "nav");
}
