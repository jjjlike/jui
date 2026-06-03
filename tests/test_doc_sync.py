#!/usr/bin/env python3
"""文档自动同步工具测试"""

import os, sys, json, tempfile, unittest
from pathlib import Path

# 添加 tools 目录到 Python path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from doc_sync import (
    parse_cpp_interface, parse_cpp_enums,
    file_hash, HashCache, update_doc_section
)


class TestCppParser(unittest.TestCase):

    def test_parse_interface_simple(self):
        code = """class ITest {
public:
    virtual void doSomething(int x) = 0;
    virtual int getValue() const = 0;
};"""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".h", delete=False,
                                          encoding="utf-8") as f:
            f.write(code)
            f.close()
            result = parse_cpp_interface(Path(f.name))
            Path(f.name).unlink()

        self.assertIn("doSomething", result)
        self.assertIn("getValue", result)
        self.assertIn("```cpp", result)

    def test_parse_interface_with_comments(self):
        code = """class IFoo {
public:
    /// Does the thing
    virtual void doThing() = 0;
    /** Returns the value */
    virtual int val() const = 0;
};"""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".h", delete=False,
                                          encoding="utf-8") as f:
            f.write(code)
            f.close()
            result = parse_cpp_interface(Path(f.name))
            Path(f.name).unlink()

        self.assertIn("Does the thing", result)
        self.assertIn("Returns the value", result)

    def test_parse_enums(self):
        code = """enum class Color { Red, Green, Blue };
enum class Size { Small = 1, Large = 2 };"""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".h", delete=False,
                                          encoding="utf-8") as f:
            f.write(code)
            f.close()
            result = parse_cpp_enums(Path(f.name))
            Path(f.name).unlink()

        self.assertIn("Color", result)
        self.assertIn("Red", result)
        self.assertIn("Green", result)
        self.assertIn("Size", result)
        self.assertIn("Small", result)

    def test_parse_private_methods_excluded(self):
        code = """class ITest {
public:
    virtual void pubMethod() = 0;
private:
    void privMethod();
};"""
        with tempfile.NamedTemporaryFile(mode="w", suffix=".h", delete=False,
                                          encoding="utf-8") as f:
            f.write(code)
            f.close()
            result = parse_cpp_interface(Path(f.name))
            Path(f.name).unlink()

        self.assertIn("pubMethod", result)
        self.assertNotIn("privMethod", result)


class TestHashCache(unittest.TestCase):

    def setUp(self):
        self.tmp = tempfile.mktemp(suffix=".json")

    def tearDown(self):
        p = Path(self.tmp)
        if p.exists():
            p.unlink()

    def test_cache_read_write(self):
        cache = HashCache(Path(self.tmp))
        cache.set("a.h", "hash123")
        self.assertEqual(cache.get("a.h"), "hash123")
        cache.save()

        cache2 = HashCache(Path(self.tmp))
        self.assertEqual(cache2.get("a.h"), "hash123")

    def test_changed(self):
        cache = HashCache(Path(self.tmp))
        cache.set("a.h", "old")
        self.assertTrue(cache.changed("a.h", "new"))
        self.assertFalse(cache.changed("a.h", "old"))


class TestDocSectionUpdate(unittest.TestCase):

    def setUp(self):
        self.tmp = tempfile.mktemp(suffix=".md")

    def tearDown(self):
        for p in [Path(self.tmp), Path(self.tmp + ".bak")]:
            if p.exists():
                p.unlink()

    def test_update_existing_section(self):
        doc = Path(self.tmp)
        doc.write_text("## Old Section\nold content\n\n## Next Section\nnext\n", encoding="utf-8")
        update_doc_section(doc, "## Old Section", "new content")
        result = doc.read_text(encoding="utf-8")
        self.assertIn("new content", result)
        self.assertIn("## Next Section", result)
        self.assertNotIn("old content", result)

    def test_update_missing_section(self):
        doc = Path(self.tmp)
        doc.write_text("## Existing\nstuff\n", encoding="utf-8")
        update_doc_section(doc, "## New Section", "new stuff")
        result = doc.read_text(encoding="utf-8")
        self.assertIn("## New Section", result)
        self.assertIn("new stuff", result)
        self.assertIn("## Existing", result)

    def test_create_new_doc(self):
        doc = Path(self.tmp)
        update_doc_section(doc, "## First", "hello world")
        self.assertTrue(doc.exists())
        self.assertIn("hello world", doc.read_text(encoding="utf-8"))


class TestFileHash(unittest.TestCase):

    def test_hash_consistent(self):
        with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as f:
            f.write("test content")
            f.close()
            h1 = file_hash(Path(f.name))
            h2 = file_hash(Path(f.name))
            Path(f.name).unlink()
        self.assertEqual(h1, h2)
        self.assertEqual(len(h1), 16)

    def test_hash_different(self):
        with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as f:
            f.write("aaa")
            f.close()
            h1 = file_hash(Path(f.name))
            Path(f.name).unlink()

        with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False) as f:
            f.write("bbb")
            f.close()
            h2 = file_hash(Path(f.name))
            Path(f.name).unlink()

        self.assertNotEqual(h1, h2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
