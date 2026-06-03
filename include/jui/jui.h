#pragma once

// ============================================================
// jui — 极简轻量 Windows C++ UI 引擎 v0.3
// A2UI 协议兼容 | D2D 渲染 | 逻辑-渲染完全分离
// 模块化架构: EventBus + DI + 接口隔离
// ============================================================

// 基础类型
#include "jui/core/value.h"
#include "jui/core/widget.h"
#include "jui/core/geometry.h"
#include "jui/core/widget_types.h"

// 接口总线
#include "jui/bus/jui_interfaces.h"

// 核心服务
#include "jui/core/data_model.h"
#include "jui/core/surface.h"
#include "jui/core/engine.h"

// 渲染
#include "jui/render/render_widget.h"
#include "jui/render/d2d_renderer.h"
#include "jui/render/widget_state.h"
#include "jui/render/jui_states.h"

// 布局
#include "jui/layout/layout.h"

// 测试工具
#include "jui/test/test.h"

// Inspector
#include "jui/inspector/inspector.h"
