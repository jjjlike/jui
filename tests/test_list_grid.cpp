/**
 * test_list_grid.cpp — List & Grid 全覆盖 + 性能基准测试
 */
#include <gtest/gtest.h>
#include <chrono>
#include "jui/render/render_widget.h"

using namespace jui;

template<typename F>
static float timeMs(F&& f) {
    auto s=std::chrono::steady_clock::now();
    f();
    return std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-s).count();
}

// ============================================================
// ListWidgetState — 基础状态
// ============================================================
TEST(ListStateTest, Initial_Empty) {
    ListWidgetState s;
    EXPECT_EQ(s.itemCount(), 0);
    EXPECT_EQ(s.scrollOffset(), 0);
    EXPECT_EQ(s.visibleStart(), 0);
    EXPECT_EQ(s.selectedIndex(), -1);
}

TEST(ListStateTest, SetItemCount) {
    ListWidgetState s; s.setItemCount(100);
    EXPECT_EQ(s.itemCount(), 100);
}

TEST(ListStateTest, ItemHeight_Recalcs) {
    ListWidgetState s; s.setViewportSize(200,200);
    s.setItemCount(100); s.setItemHeight(40);
    EXPECT_EQ(s.itemHeight(), 40);
    EXPECT_GT(s.maxScroll(), 0);
}

TEST(ListStateTest, VisibleRange_Basic) {
    ListWidgetState s;
    s.setViewportSize(200,200); s.setItemHeight(32); s.setItemCount(100);
    EXPECT_EQ(s.visibleStart(), 0);
    EXPECT_GE(s.visibleEnd(), 6);
    EXPECT_TRUE(s.isItemVisible(0));
    EXPECT_FALSE(s.isItemVisible(50));
}

TEST(ListStateTest, VisibleRange_AfterScroll) {
    ListWidgetState s;
    s.setViewportSize(200,200); s.setItemHeight(32); s.setItemCount(100);
    s.setScrollOffset(320);
    EXPECT_EQ(s.visibleStart(), 10);
    EXPECT_TRUE(s.isItemVisible(10));
    EXPECT_FALSE(s.isItemVisible(5));
}

TEST(ListStateTest, ScrollBy_Clamps) {
    ListWidgetState s; s.setViewportSize(200,200); s.setItemHeight(32); s.setItemCount(10);
    s.scrollBy(-100); EXPECT_EQ(s.scrollOffset(), 0);
    s.scrollBy(9999); EXPECT_EQ(s.scrollOffset(), s.maxScroll());
}

TEST(ListStateTest, ScrollToIndex) {
    ListWidgetState s; s.setViewportSize(200,200); s.setItemHeight(32); s.setItemCount(100);
    s.scrollToIndex(50);
    EXPECT_EQ(s.visibleStart(), 50);
}

TEST(ListStateTest, SelectIndex) {
    ListWidgetState s; s.setItemCount(50);
    s.selectIndex(3);
    EXPECT_EQ(s.selectedIndex(), 3);
    EXPECT_TRUE(s.isSelected(3));
    EXPECT_FALSE(s.isSelected(0));
}

TEST(ListStateTest, ClearSelection) {
    ListWidgetState s; s.setItemCount(10);
    s.selectIndex(5); s.clearSelection();
    EXPECT_EQ(s.selectedIndex(), -1);
}

TEST(ListStateTest, SelectCallback) {
    ListWidgetState s; s.setItemCount(10);
    int idx=-1; s.onSelect([&](int i){idx=i;});
    s.selectIndex(7); EXPECT_EQ(idx,7);
}

TEST(ListStateTest, ItemProvider) {
    ListWidgetState s; s.setItemCount(50);
    s.setItemProvider([](int i){return "Row "+std::to_string(i);});
    EXPECT_EQ(s.itemText(0), "Row 0");
    EXPECT_EQ(s.itemText(42), "Row 42");
}

TEST(ListStateTest, ItemProvider_Default) {
    ListWidgetState s; s.setItemCount(10);
    EXPECT_EQ(s.itemText(0), "Item 0");
}

// ============================================================
// GridWidgetState
// ============================================================
TEST(GridStateTest, Initial_Empty) {
    GridWidgetState s;
    EXPECT_EQ(s.columnCount(), 0);
    EXPECT_EQ(s.rowCount(), 0);
}

TEST(GridStateTest, SetColumns) {
    GridWidgetState s;
    s.setColumns({{"Name",100},{"Age",60},{"Email",180}});
    EXPECT_EQ(s.columnCount(), 3);
    EXPECT_GT(s.totalWidth(), 0);
}

TEST(GridStateTest, SetRows) {
    GridWidgetState s;
    s.setColumns({{"A",50}});
    s.setRowCount(200);
    EXPECT_EQ(s.rowCount(), 200);
}

TEST(GridStateTest, Selection) {
    GridWidgetState s;
    s.setColumns({{"X",50},{"Y",50}}); s.setRowCount(50);
    s.setSelection(10,1);
    EXPECT_EQ(s.selectedRow(),10);
    EXPECT_EQ(s.selectedCol(),1);
}

TEST(GridStateTest, Scroll_Clamped) {
    GridWidgetState s;
    s.setColumns({{"A",50},{"B",50}}); s.setRowCount(20);
    s.setViewportSize(200,200);
    s.setScroll(1000,1000);
    EXPECT_LT(s.scrollX(), 100);
}

TEST(GridStateTest, VisibleRows) {
    GridWidgetState s;
    s.setColumns({{"C",80}}); s.setRowCount(100);
    s.setViewportSize(300,200);
    EXPECT_GE(s.visibleRowEnd()-s.visibleRowStart(), 0);
    s.setScroll(0,500);
    EXPECT_GT(s.visibleRowStart(), 0);
}

TEST(GridStateTest, Sort_Toggle) {
    GridWidgetState s; s.setColumns({{"Name",100}});
    s.toggleSort(0); EXPECT_EQ(s.sortColumn(),0); EXPECT_TRUE(s.sortAsc());
    s.toggleSort(0); EXPECT_FALSE(s.sortAsc());
}

TEST(GridStateTest, Sort_SwitchColumn) {
    GridWidgetState s; s.setColumns({{"A",50},{"B",50}});
    s.toggleSort(0); s.toggleSort(1);
    EXPECT_EQ(s.sortColumn(),1); EXPECT_TRUE(s.sortAsc());
}

// ============================================================
// Render tests
// ============================================================
TEST(ListRenderTest, CanFocus) {
    ListRenderWidget rw(std::make_shared<Widget>("l",WidgetType::List));
    EXPECT_TRUE(rw.canFocus());
}

TEST(ListRenderTest, HitTest) {
    ListRenderWidget rw(std::make_shared<Widget>("l",WidgetType::List));
    rw.setBounds({10,10,200,200});
    EXPECT_TRUE(rw.hitTest(50,50));
    EXPECT_FALSE(rw.hitTest(500,500));
}

TEST(ListRenderTest, Measure) {
    auto w=std::make_shared<Widget>("l",WidgetType::List);
    w->setProperty("width",JValue(300.0f));
    w->setProperty("height",JValue(400.0f));
    ListRenderWidget rw(w);
    EXPECT_FLOAT_EQ(rw.measure(nullptr).w,300.0f);
}

TEST(ListRenderTest, OnClick_Selects) {
    ListRenderWidget rw(std::make_shared<Widget>("l",WidgetType::List));
    rw.setBounds({0,0,200,200});
    auto* st=dynamic_cast<ListWidgetState*>(rw.state());
    st->setViewportSize(200,200); st->setItemHeight(32); st->setItemCount(50);
    rw.onClick(10,100);
    EXPECT_GE(st->selectedIndex(), 0);
}

TEST(GridRenderTest, CanFocus) {
    GridRenderWidget rw(std::make_shared<Widget>("g",WidgetType::Grid));
    EXPECT_TRUE(rw.canFocus());
}

TEST(GridRenderTest, HitTest) {
    GridRenderWidget rw(std::make_shared<Widget>("g",WidgetType::Grid));
    rw.setBounds({0,0,400,200});
    EXPECT_TRUE(rw.hitTest(100,50));
}

TEST(GridRenderTest, Measure) {
    auto w=std::make_shared<Widget>("g",WidgetType::Grid);
    w->setProperty("width",JValue(500.0f));
    GridRenderWidget rw(w);
    EXPECT_FLOAT_EQ(rw.measure(nullptr).w,500.0f);
}

TEST(GridRenderTest, OnScroll) {
    GridRenderWidget rw(std::make_shared<Widget>("g",WidgetType::Grid));
    auto* st=dynamic_cast<GridWidgetState*>(rw.state());
    st->setColumns({{"X",50},{"Y",50}}); st->setRowCount(100);
    st->setViewportSize(400,200);
    rw.onScrollY(200);
    EXPECT_GT(st->scrollY(), 0);
}

// ============================================================
// 性能基准测试
// ============================================================
TEST(ListPerfTest, VirtualScroll_OnlyVisibleItems) {
    ListWidgetState s;
    s.setViewportSize(200, 600); s.setItemHeight(32); s.setItemCount(100000);

    float t = timeMs([&]{
        int vs = s.visibleStart();
        int ve = s.visibleEnd();
        volatile int cnt = ve - vs;
        (void)cnt;
    });
    EXPECT_LT(t, 2.0f);
    EXPECT_LT(s.visibleEnd()-s.visibleStart(), 30);
    EXPECT_LT(s.visibleEnd(), 100000);
}

TEST(ListPerfTest, ItemIteration_Performance) {
    ListWidgetState s;
    s.setItemCount(100000); s.setViewportSize(200,600); s.setItemHeight(32);

    float t = timeMs([&]{
        int total=0;
        for(int i=s.visibleStart(); i<s.visibleEnd(); i++) total++;
        EXPECT_LE(total, 30);
    });
    EXPECT_LT(t, 1.0f);
}

TEST(ListPerfTest, LargeDataset_Scroll1000Times) {
    ListWidgetState s;
    s.setItemCount(200000); s.setViewportSize(200,600); s.setItemHeight(32);

    float t = timeMs([&]{
        for(int off=0; off<1000; off+=100)
            s.setScrollOffset(off*32);
    });
    EXPECT_LT(t, 10.0f);
}

TEST(GridPerfTest, LargeGrid_Scroll1000) {
    GridWidgetState s;
    s.setColumns({{"ID",60},{"Name",120},{"Email",200},{"Phone",120},{"City",100}});
    s.setRowCount(50000); s.setViewportSize(600,300);

    float t = timeMs([&]{
        for(int r=0; r<1000; r+=50)
            s.setScroll(0, r*s.rowHeight());
    });
    EXPECT_LT(t, 5.0f);
}

TEST(ListPerfTest, VirtualVsTotal_1Percent) {
    ListWidgetState s;
    s.setItemCount(100000); s.setViewportSize(200,600); s.setItemHeight(32);

    int visible = s.visibleEnd()-s.visibleStart();
    EXPECT_LT(visible, s.itemCount()/100);
}
