#!/usr/bin/env python3
"""
Scan C:\\ENESS_Projects and build a master JSON table of projects + scripts.

Rules (matches scheduler Script Library):
  - only top-level folders whose names start with 4 digits
  - skips incremental saves ending in _<number>.pxl
  - pairs content scripts with a sibling *_client.pxl when present

Each project row includes git upstream (origin URL) when available.

Usage:
  python scan_eness_projects.py
  python scan_eness_projects.py --out eness_projects_master.json
  python scan_eness_projects.py --root D:\\other\\path --json
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import asdict, dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urlparse, urlunparse

DEFAULT_ROOT = Path(r"C:\ENESS_Projects")
DEFAULT_OUT = Path(__file__).resolve().parent / "eness_projects_master.json"
MAX_DEPTH = 6
GIT_SEARCH_DEPTH = 4
INCREMENTAL_RE = re.compile(r"_\d+$")
PROJECT_FOLDER_RE = re.compile(r"^\d{4}")


@dataclass
class GitInfo:
    root: str | None = None
    upstream: str | None = None
    branch: str | None = None


@dataclass
class ScriptEntry:
    id: str
    name: str
    content_path: str
    content_path_relative: str
    client_path: str
    client_path_relative: str
    source: str = "scan"


@dataclass
class ProjectEntry:
    folder: str
    path: str
    git: GitInfo = field(default_factory=GitInfo)
    script_count: int = 0
    scripts: list[ScriptEntry] = field(default_factory=list)


def slugify(text: str) -> str:
    out = []
    last_dash = False
    for ch in text.lower():
        if ch.isalnum():
            out.append(ch)
            last_dash = False
        elif not last_dash:
            out.append("-")
            last_dash = True
    slug = "".join(out).strip("-")
    return slug or "script"


def beautify(text: str) -> str:
    return text.replace("_", " ").replace("-", " ")


def normalize_upstream(url: str) -> str:
    """Strip embedded credentials from remotes like https://user@host/repo.git."""
    url = url.strip()
    if not url:
        return url
    if url.startswith("git@"):
        return url
    parsed = urlparse(url)
    if not parsed.scheme:
        return url
    netloc = parsed.hostname or ""
    if parsed.port:
        netloc = f"{netloc}:{parsed.port}"
    return urlunparse((parsed.scheme, netloc, parsed.path, parsed.params, parsed.query, parsed.fragment))


def run_git(repo: Path, *args: str) -> str | None:
    try:
        result = subprocess.run(
            ["git", "-C", str(repo), *args],
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    if result.returncode != 0:
        return None
    return result.stdout.strip() or None


def find_git_root(project_dir: Path) -> Path | None:
    if (project_dir / ".git").exists():
        return project_dir

    best: Path | None = None
    best_depth = 10**9
    for path in project_dir.rglob(".git"):
        repo_root = path.parent
        try:
            depth = len(repo_root.relative_to(project_dir).parts)
        except ValueError:
            continue
        if depth > GIT_SEARCH_DEPTH:
            continue
        if depth < best_depth:
            best = repo_root
            best_depth = depth
    return best


def read_git_info(project_dir: Path) -> GitInfo:
    repo = find_git_root(project_dir)
    if repo is None:
        return GitInfo()

    upstream = run_git(repo, "remote", "get-url", "origin")
    branch = run_git(repo, "branch", "--show-current")
    return GitInfo(
        root=str(repo.resolve()),
        upstream=normalize_upstream(upstream) if upstream else None,
        branch=branch,
    )


def is_project_folder(name: str) -> bool:
    return bool(PROJECT_FOLDER_RE.match(name))


def is_incremental_save(path: Path) -> bool:
    return bool(INCREMENTAL_RE.search(path.stem))


def is_client_pxl(path: Path) -> bool:
    return path.stem.endswith("_client")


def client_sibling(content_path: Path) -> Path:
    return content_path.with_name(f"{content_path.stem}_client{content_path.suffix}")


def rel_or_abs(project_root: Path, path: Path) -> str:
    try:
        return str(path.resolve().relative_to(project_root.resolve())).replace("\\", "/")
    except ValueError:
        return str(path.resolve())


def display_name(project_root: Path, pxl_path: Path) -> str:
    project_label = beautify(project_root.name)
    script_label = beautify(pxl_path.stem)
    if script_label == project_label:
        return project_label
    return f"{project_label} - {script_label}"


def collect_scripts(project_root: Path, max_depth: int = MAX_DEPTH) -> list[ScriptEntry]:
    found: list[ScriptEntry] = []
    if not project_root.is_dir():
        return found

    for path in project_root.rglob("*.pxl"):
        try:
            rel = path.relative_to(project_root)
        except ValueError:
            continue

        if len(rel.parts) > max_depth:
            continue
        if is_incremental_save(path):
            continue
        if is_client_pxl(path):
            continue

        client = client_sibling(path)
        client_abs = ""
        client_rel = ""
        if client.is_file() and not is_incremental_save(client):
            client_abs = str(client.resolve())
            client_rel = rel_or_abs(project_root, client)

        rel_id = str(rel).replace("\\", "/")
        found.append(
            ScriptEntry(
                id=slugify(f"{project_root.name}-{rel_id}"),
                name=display_name(project_root, path),
                content_path=str(path.resolve()),
                content_path_relative=rel_id,
                client_path=client_abs,
                client_path_relative=client_rel,
            )
        )

    found.sort(key=lambda e: e.content_path.lower())
    return found


def scan_root(root: Path) -> list[ProjectEntry]:
    root = root.resolve()
    if not root.is_dir():
        raise FileNotFoundError(f"Scan root does not exist: {root}")

    projects: list[ProjectEntry] = []
    for child in sorted(root.iterdir(), key=lambda p: p.name.lower()):
        if not child.is_dir() or not is_project_folder(child.name):
            continue

        scripts = collect_scripts(child)
        projects.append(
            ProjectEntry(
                folder=child.name,
                path=str(child.resolve()),
                git=read_git_info(child),
                script_count=len(scripts),
                scripts=scripts,
            )
        )

    projects.sort(key=lambda p: p.folder.lower())
    return projects


def split_by_upstream(projects: list[ProjectEntry]) -> tuple[list[ProjectEntry], list[ProjectEntry]]:
    included = [p for p in projects if p.git.upstream]
    excluded = [p for p in projects if not p.git.upstream]
    return included, excluded


def build_master_table(root: Path, included: list[ProjectEntry]) -> dict:
    script_count = sum(p.script_count for p in included)
    return {
        "schema": "eness_projects_master/v1",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "scan_root": str(root.resolve()),
        "project_count": len(included),
        "script_count": script_count,
        "projects": [asdict(p) for p in included],
    }


def print_excluded(excluded: list[ProjectEntry]) -> None:
    if not excluded:
        return
    print(f"Excluded {len(excluded)} project(s) with no git upstream:")
    for project in excluded:
        print(f"  - {project.folder}  ({project.script_count} scripts)")
        print(f"      path: {project.path}")
    print()


def print_summary(table: dict) -> None:
    print(f"Scan root: {table['scan_root']}")
    print(f"Projects:  {table['project_count']}")
    print(f"Scripts:   {table['script_count']}")
    print()

    for project in table["projects"]:
        upstream = project["git"]["upstream"] or "(no upstream)"
        print(f"- {project['folder']}  [{project['script_count']} scripts]")
        print(f"    upstream: {upstream}")
        if project["script_count"]:
            first = project["scripts"][0]
            print(f"    example:  {first['name']}")
            print(f"              {first['content_path']}")
        print()


def main() -> int:
    parser = argparse.ArgumentParser(description="Build master JSON table of ENESS projects and .pxl scripts.")
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT, help=f"Scan root (default: {DEFAULT_ROOT})")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help=f"Output JSON path (default: {DEFAULT_OUT})")
    parser.add_argument("--json", action="store_true", help="Also print full JSON to stdout")
    parser.add_argument("--summary", action="store_true", help="Print summary table instead of write-only mode")
    args = parser.parse_args()

    try:
        projects = scan_root(args.root)
    except FileNotFoundError as exc:
        print(exc, file=sys.stderr)
        return 1

    included, excluded = split_by_upstream(projects)
    table = build_master_table(args.root, included)
    text = json.dumps(table, indent=2)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding="utf-8")
    print(f"Wrote master table to {args.out}")
    print_excluded(excluded)

    if args.json:
        print(text)
    elif args.summary:
        print_summary(table)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
