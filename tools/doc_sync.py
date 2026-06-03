#!/usr/bin/env python3
"""
JUI 文档自动同步工具 (Documentation Auto-Sync Tool)

功能:
  1. 监听核心代码文件变更（API 接口、类型定义、配置文件）
  2. 自动提取 C++ 注释和类型定义生成文档内容
  3. 更新目标文档指定章节，保持与代码版本一致
  4. 文件哈希缓存，仅变更时触发同步
  5. 异常日志 + 回滚机制

用法:
  python tools/doc_sync.py [--check] [--force] [--watch]

选项:
  --check   仅检查是否有文件变更，不更新文档（CI 模式）
  --force   忽略缓存，强制全量同步
  --watch   持续监听文件变更（开发模式）
"""

import os, sys, json, hashlib, re, time
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional, Tuple

PROJECT_ROOT = Path(__file__).resolve().parent.parent
CONFIG_PATH = PROJECT_ROOT / "tools" / "doc_sync_config.json"


# ═══════════════════════════════════════════════════════════════
# 日志系统
# ═══════════════════════════════════════════════════════════════

class Logger:
    def __init__(self, log_path: Path, max_lines: int = 500):
        self.log_path = log_path
        self.max_lines = max_lines

    def _write(self, level: str, msg: str):
        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        line = f"[{ts}] [{level}] {msg}"
        print(line)
        if self.log_path:
            with open(self.log_path, "a", encoding="utf-8") as f:
                f.write(line + "\n")

    def info(self, msg): self._write("INFO", msg)
    def warn(self, msg): self._write("WARN", msg)
    def error(self, msg): self._write("ERROR", msg)

    @staticmethod
    def null() -> "Logger":
        """返回一个空日志（仅打印到 stdout）"""
        import tempfile
        return Logger(Path(tempfile.gettempdir()) / "__null_log__")
    def rotate(self):
        p = self.log_path
        if p.exists() and p.stat().st_size > 1024 * 1024:
            bak = p.with_suffix(".log.bak")
            import shutil; shutil.move(str(p), str(bak))


# ═══════════════════════════════════════════════════════════════
# 哈希缓存
# ═══════════════════════════════════════════════════════════════

class HashCache:
    def __init__(self, cache_path: Path):
        self.path = cache_path
        self.data: Dict[str, str] = {}
        self._load()

    def _load(self):
        if self.path.exists():
            try:
                self.data = json.loads(self.path.read_text(encoding="utf-8"))
            except (json.JSONDecodeError, OSError):
                self.data = {}

    def save(self):
        self.path.write_text(json.dumps(self.data, indent=2), encoding="utf-8")

    def get(self, filepath: str) -> str:
        return self.data.get(filepath, "")

    def set(self, filepath: str, hash_val: str):
        self.data[filepath] = hash_val

    def changed(self, filepath: str, new_hash: str) -> bool:
        return self.get(filepath) != new_hash


# ═══════════════════════════════════════════════════════════════
# 文件哈希
# ═══════════════════════════════════════════════════════════════

def file_hash(filepath: Path, algo: str = "sha256") -> str:
    h = hashlib.new(algo)
    h.update(filepath.read_bytes())
    return h.hexdigest()[:16]


def glob_files(root: Path, pattern: str) -> List[Path]:
    """支持通配符的 glob"""
    if pattern.endswith("/"):
        pattern = pattern.rstrip("/") + "/*"
    if "*" in pattern:
        return sorted(root.glob(pattern))
    p = root / pattern
    return [p] if p.exists() else []


# ═══════════════════════════════════════════════════════════════
# C++ 代码解析
# ═══════════════════════════════════════════════════════════════

def parse_cpp_interface(filepath: Path) -> str:
    """解析 C++ 接口文件，提取方法签名和注释"""
    content = filepath.read_text(encoding="utf-8", errors="replace")
    lines = content.split("\n")

    result = [f"\n### `{filepath.name}`"]
    in_class = False
    class_name = ""
    current_comment = []
    methods = []

    for line in lines:
        stripped = line.strip()

        # 跟踪类声明
        m_cls = re.match(r"class\s+(\w+)\s*[{:]", stripped)
        if m_cls:
            class_name = m_cls.group(1)
            in_class = True
            if current_comment:
                result.append("  " + " ".join(current_comment).replace("*/", "").replace("/**", "").replace(" *", "").strip())
                current_comment = []
            continue

        # 类结束
        if in_class and stripped == "};":
            in_class = False
            continue

        # 收集注释
        if stripped.startswith("/**") or stripped.startswith("///"):
            current_comment.append(stripped)
            continue
        if current_comment and (stripped.startswith("*") or stripped.startswith("//")):
            current_comment.append(stripped)
            continue

        # 检测虚方法
        if in_class and "virtual" in stripped:
            # 合并注释
            comment = ""
            if current_comment:
                comment = " ".join(
                    c.replace("/**", "").replace("*/", "").replace("///", "")
                     .replace("* ", "").replace(" *", "").strip()
                    for c in current_comment
                )
                current_comment = []

            # 提取方法签名
            sig = re.sub(r"\s*virtual\s+", "", stripped)
            sig = re.sub(r"\s*=\s*0\s*;", ";", sig)
            sig = re.sub(r"\s*override\s*", "", sig)
            sig = sig.strip()

            methods.append((comment, sig))
        else:
            current_comment = []

    if methods:
        result.append(f"```cpp")
        for comment, sig in methods:
            if comment:
                result.append(f"// {comment}")
            result.append(f"{sig};")
        result.append("```")

    return "\n".join(result) if methods else ""


def parse_cpp_enums(filepath: Path) -> str:
    """解析 C++ 枚举"""
    content = filepath.read_text(encoding="utf-8", errors="replace")
    result = []

    for m in re.finditer(r"enum\s+class\s+(\w+)\s*\{([^}]+)\}", content):
        name = m.group(1)
        values = [v.strip().rstrip(",") for v in m.group(2).split("\n") if v.strip()]
        result.append(f"\n#### `{name}`")
        for v in values:
            result.append(f"  - `{v}`")

    return "\n".join(result) if result else ""


# ═══════════════════════════════════════════════════════════════
# 文档生成器
# ═══════════════════════════════════════════════════════════════

def generate_interface_doc(filepaths: List[Path]) -> str:
    """从接口文件生成文档"""
    parts = []
    for fp in filepaths:
        parts.append(parse_cpp_interface(fp))
        parts.append(parse_cpp_enums(fp))
    return "\n".join(p for p in parts if p)


def generate_enum_changelog(filepaths: List[Path]) -> str:
    """从枚举生成变更日志条目"""
    now = datetime.now().strftime("%Y-%m-%d")
    parts = [f"\n### {now} — Widget 类型快照"]
    for fp in filepaths:
        parts.append(parse_cpp_enums(fp))
    return "\n".join(parts) if len(parts) > 1 else ""


def generate_api_changelog(filepaths: List[Path]) -> str:
    """从引擎头文件生成 API 变更日志"""
    now = datetime.now().strftime("%Y-%m-%d")
    parts = [f"\n### {now} — 引擎 API 快照"]
    for fp in filepaths:
        parts.append(parse_cpp_interface(fp))
    return "\n".join(parts) if len(parts) > 1 else ""


def generate_module_list(patterns: List[str]) -> str:
    """从文件模式生成模块清单"""
    parts = ["\n| 模块 | 文件 | 说明 |", "|------|------|------|"]
    for pat in patterns:
        files = glob_files(PROJECT_ROOT, pat)
        for f in files:
            name = f.stem
            desc = f"`{f.relative_to(PROJECT_ROOT).as_posix()}`"
            parts.append(f"| {name} | {desc} | 状态类 |")
    return "\n".join(parts)


GENERATORS = {
    "interface_doc": generate_interface_doc,
    "enum_changelog": generate_enum_changelog,
    "api_changelog": generate_api_changelog,
    "module_list": generate_module_list,
}


# ═══════════════════════════════════════════════════════════════
# 文档更新
# ═══════════════════════════════════════════════════════════════

def update_doc_section(doc_path: Path, section_marker: str, new_content: str) -> bool:
    """更新文档中指定章节的内容"""
    if not doc_path.exists():
        logger.warn(f"Creating new document: {doc_path}")
        doc_path.write_text(f"{section_marker}\n{new_content}\n", encoding="utf-8")
        return True

    content = doc_path.read_text(encoding="utf-8")
    lines = content.split("\n")

    # 查找 section marker
    section_start = -1
    for i, line in enumerate(lines):
        if line.strip() == section_marker.strip():
            section_start = i
            break

    if section_start < 0:
        # 未找到 → 追加到文件末尾
        logger.info(f"Section '{section_marker}' not found, appending to {doc_path}")
        content += f"\n{section_marker}\n{new_content}\n"
    else:
        # 替换该 section 到下一个同级 section 之间的内容
        section_end = len(lines)
        level = len(section_marker) - len(section_marker.lstrip("#"))
        prefix = "#" * level + " "
        for i in range(section_start + 1, len(lines)):
            if lines[i].startswith(prefix) and not lines[i].startswith("#" * (level + 1)):
                section_end = i
                break

        new_lines = lines[:section_start + 1] + [new_content] + lines[section_end:]
        content = "\n".join(new_lines)

    # 备份原内容
    bak_path = doc_path.with_suffix(doc_path.suffix + ".bak")
    doc_path.rename(bak_path)

    try:
        doc_path.write_text(content, encoding="utf-8")
        bak_path.unlink()  # 成功则删除备份
        return True
    except OSError as e:
        # 回滚
        bak_path.rename(doc_path)
        logger.error(f"Failed to write {doc_path}: {e}")
        return False


# ═══════════════════════════════════════════════════════════════
# 主流程
# ═══════════════════════════════════════════════════════════════

logger = Logger.null()  # 默认 null logger，main() 中替换为真实 logger


def load_config() -> dict:
    try:
        return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as e:
        print(f"FATAL: Cannot load config: {e}")
        sys.exit(1)


def run_sync(config: dict, force: bool = False) -> Dict[str, int]:
    """执行同步，返回统计 {updated: N, skipped: N, failed: N}"""
    stats = {"updated": 0, "skipped": 0, "failed": 0}
    settings = config.get("globalSettings", {})
    cache = HashCache(PROJECT_ROOT / settings.get("cacheFile", ".doc_sync_cache.json"))

    for watcher in config.get("watchers", []):
        name = watcher["name"]
        patterns = watcher.get("watchPatterns", [])
        target_doc = PROJECT_ROOT / watcher["targetDoc"]
        section = watcher["targetSection"]
        generator_name = watcher.get("generator", "interface_doc")

        # 收集所有匹配文件
        files: List[Path] = []
        for pat in patterns:
            files.extend(glob_files(PROJECT_ROOT, pat))

        if not files:
            logger.info(f"[{name}] No files matched patterns {patterns}")
            stats["skipped"] += 1
            continue

        # 检查是否有文件变更
        has_changes = force
        for fp in files:
            if not fp.exists():
                continue
            h = file_hash(fp)
            if cache.changed(str(fp.relative_to(PROJECT_ROOT)), h):
                has_changes = True
            cache.set(str(fp.relative_to(PROJECT_ROOT)), h)

        if not has_changes:
            logger.info(f"[{name}] No changes, skipped")
            stats["skipped"] += 1
            continue

        # 生成文档内容
        try:
            gen = GENERATORS.get(generator_name)
            if generator_name == "module_list":
                new_content = gen(patterns) if gen else ""
            else:
                new_content = gen(files) if gen else ""
        except Exception as e:
            logger.error(f"[{name}] Generator '{generator_name}' failed: {e}")
            stats["failed"] += 1
            continue

        if not new_content.strip():
            logger.warn(f"[{name}] Generated empty content, skipped")
            stats["skipped"] += 1
            continue

        # 更新目标文档
        ok = update_doc_section(target_doc, section, new_content)
        if ok:
            logger.info(f"[{name}] Updated {target_doc.relative_to(PROJECT_ROOT)} → '{section}'")
            stats["updated"] += 1
        else:
            stats["failed"] += 1

    cache.save()
    return stats


def main():
    config = load_config()
    settings = config.get("globalSettings", {})
    log_path = PROJECT_ROOT / settings.get("logFile", "doc_sync.log")

    global logger
    logger = Logger(log_path, settings.get("maxLogLines", 500))
    logger.rotate()
    logger.info("=== Document Auto-Sync Started ===")

    args = sys.argv[1:]
    force = "--force" in args
    check_only = "--check" in args
    watch_mode = "--watch" in args

    if watch_mode:
        logger.info("Watch mode — monitoring file changes (Ctrl+C to stop)")
        try:
            while True:
                stats = run_sync(config, force=False)
                if stats["updated"] > 0:
                    logger.info(f"Sync done: updated={stats['updated']}, "
                                f"skipped={stats['skipped']}, failed={stats['failed']}")
                time.sleep(5)
        except KeyboardInterrupt:
            logger.info("Watch mode stopped")
        return

    stats = run_sync(config, force=force)

    if check_only:
        if stats["updated"] > 0 or stats["failed"] > 0:
            logger.warn(f"Check mode: {stats['updated']} files need update, "
                        f"{stats['failed']} failures")
            sys.exit(1)
        else:
            logger.info("Check mode: all documents up to date")
            sys.exit(0)

    logger.info(f"Sync complete: updated={stats['updated']}, "
                f"skipped={stats['skipped']}, failed={stats['failed']}")

    if stats["failed"] > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
