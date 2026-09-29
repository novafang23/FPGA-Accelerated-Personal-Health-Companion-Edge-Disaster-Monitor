"""
VALOR Multi-Agent Code Auditor
================================
A real Google Antigravity SDK multi-agent auditor for the SIH26181 mixed-stack
repository (Verilog + C + Python + HTML).

Architecture:
  Root Agent (orchestrator)
    ├── hardware_auditor   — scans *.v files for CDC, blocking assigns, latch inference
    ├── systems_auditor    — scans *.c/*.h for buffer overflows, data races, FPU misuse
    ├── backend_auditor    — scans *.py for security vectors, resource leaks, perf issues
    └── frontend_auditor   — scans *.html for offline deps, a11y, XSS vectors

Usage:
  1. Set GEMINI_API_KEY in your environment or .env file
  2. python scripts/valor_code_auditor.py
  3. The structured report is saved to reports/antigravity_audit_report.md

Requirements:
  pip install google-antigravity pydantic python-dotenv
"""

import asyncio
import glob
import os
import sys
from pathlib import Path
from datetime import datetime

try:
    from dotenv import load_dotenv
    load_dotenv()
except ImportError:
    pass  # python-dotenv is optional; env vars can be set directly

try:
    from google.antigravity import Agent, LocalAgentConfig, types
    import pydantic
except ImportError:
    print("ERROR: Required packages not installed.")
    print("Run: pip install google-antigravity pydantic python-dotenv")
    sys.exit(1)


# ─────────────────────────────────────────────────────────────────────────────
# 1. Structured Output Schema (Pydantic)
# ─────────────────────────────────────────────────────────────────────────────

class AuditIssue(pydantic.BaseModel):
    """A single code quality issue found by an auditor agent."""
    severity: str = pydantic.Field(
        description="One of: CRITICAL, WARNING, OPTIMISATION"
    )
    file: str = pydantic.Field(
        description="Relative filepath where the issue was found"
    )
    line_hint: str = pydantic.Field(
        description="Approximate line number or range, or 'N/A' if file-wide"
    )
    title: str = pydantic.Field(
        description="One-line summary of the issue"
    )
    detail: str = pydantic.Field(
        description="Explanation of why this is a problem and how to fix it"
    )

class AuditReport(pydantic.BaseModel):
    """Structured report from a single domain auditor."""
    domain: str = pydantic.Field(
        description="The domain: hardware, systems, backend, or frontend"
    )
    files_scanned: int
    issues: list[AuditIssue]
    top_structural_risk: str = pydantic.Field(
        description="The single most critical structural risk for this layer"
    )


# ─────────────────────────────────────────────────────────────────────────────
# 2. File Discovery (custom tools the agents can call)
# ─────────────────────────────────────────────────────────────────────────────

PROJECT_ROOT = Path(__file__).resolve().parent.parent

# Exclusion patterns for generated/vendored code
EXCLUDE_PATTERNS = [
    "vivado_project", "simulation-models", "ip_user_files",
    "build", "node_modules", "__pycache__", ".git"
]


def _should_exclude(filepath: str) -> bool:
    """Check if a filepath matches any exclusion pattern."""
    return any(pat in filepath for pat in EXCLUDE_PATTERNS)


def discover_verilog_files() -> str:
    """Discover all Verilog (.v, .sv) source files in hardware/.

    Returns:
        A newline-separated list of relative file paths.
    """
    files = []
    for ext in ("*.v", "*.sv"):
        for f in glob.glob(str(PROJECT_ROOT / "hardware" / "**" / ext), recursive=True):
            if not _should_exclude(f):
                files.append(os.path.relpath(f, PROJECT_ROOT))
    return "\n".join(files) if files else "No Verilog files found."


def discover_c_files() -> str:
    """Discover all C/H source files in firmware/ (excluding build artifacts).

    Returns:
        A newline-separated list of relative file paths.
    """
    files = []
    for ext in ("*.c", "*.h"):
        for f in glob.glob(str(PROJECT_ROOT / "firmware" / "**" / ext), recursive=True):
            if not _should_exclude(f):
                files.append(os.path.relpath(f, PROJECT_ROOT))
    return "\n".join(files) if files else "No C/H files found."


def discover_python_files() -> str:
    """Discover all Python scripts in scripts/ (excluding this file).

    Returns:
        A newline-separated list of relative file paths.
    """
    files = []
    for f in glob.glob(str(PROJECT_ROOT / "scripts" / "**" / "*.py"), recursive=True):
        if not _should_exclude(f) and "valor_code_auditor" not in f:
            files.append(os.path.relpath(f, PROJECT_ROOT))
    return "\n".join(files) if files else "No Python files found."


def discover_html_files() -> str:
    """Discover all HTML files in the project (excluding build artifacts).

    Returns:
        A newline-separated list of relative file paths.
    """
    files = []
    for f in glob.glob(str(PROJECT_ROOT / "**" / "*.html"), recursive=True):
        if not _should_exclude(f):
            files.append(os.path.relpath(f, PROJECT_ROOT))
    return "\n".join(files) if files else "No HTML files found."


def read_source_file(relative_path: str) -> str:
    """Read the contents of a source file given its path relative to the project root.

    Args:
        relative_path: Path relative to the project root (e.g., 'firmware/core/spo2_engine.c').

    Returns:
        The file contents as a string, or an error message if the file is too large or missing.
    """
    full_path = PROJECT_ROOT / relative_path
    if not full_path.exists():
        return f"ERROR: File not found: {relative_path}"
    size = full_path.stat().st_size
    if size > 50_000:
        return f"ERROR: File too large ({size} bytes). Skipping to stay within context limits."
    try:
        return full_path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        return f"ERROR reading {relative_path}: {e}"


# ─────────────────────────────────────────────────────────────────────────────
# 3. Subagent Definitions
# ─────────────────────────────────────────────────────────────────────────────

hardware_auditor = types.SubagentConfig(
    name="hardware_auditor",
    description=(
        "Verilog/SystemVerilog RTL quality auditor. Scans for: "
        "clock domain crossing (CDC) violations, blocking assignments in "
        "sequential logic, missing resets, latch inference from incomplete "
        "case statements, hardcoded magic numbers, and synthesis-hostile constructs."
    ),
    capabilities=types.SubagentCapabilities(
        agent_behavior=types.AgentBehavior.AUTONOMOUS,
    ),
)

systems_auditor = types.SubagentConfig(
    name="systems_auditor",
    description=(
        "C/C++ embedded firmware memory safety auditor. Scans for: "
        "buffer overflows (strcpy/sprintf/gets), data races between FreeRTOS "
        "tasks sharing buffers, use of double-precision float on single-precision "
        "FPU hardware (ESP32-S3), dangling pointers, missing NULL checks after "
        "malloc, and interrupt-unsafe shared state access."
    ),
    capabilities=types.SubagentCapabilities(
        agent_behavior=types.AgentBehavior.AUTONOMOUS,
    ),
)

backend_auditor = types.SubagentConfig(
    name="backend_auditor",
    description=(
        "Python tooling security and performance auditor. Scans for: "
        "hardcoded API keys, use of eval()/exec()/shell=True, unclosed file "
        "handles, blocking synchronous I/O that should be async, missing error "
        "handling around network calls, and type-hinting gaps."
    ),
    capabilities=types.SubagentCapabilities(
        agent_behavior=types.AgentBehavior.AUTONOMOUS,
    ),
)

frontend_auditor = types.SubagentConfig(
    name="frontend_auditor",
    description=(
        "HTML/CSS accessibility and offline-resilience auditor. Scans for: "
        "external CDN dependencies (critical for edge/disaster deployment), "
        "missing ARIA labels, missing alt text on images, form inputs without "
        "labels, inline scripts (XSS vectors), WCAG 2.1 contrast violations, "
        "and broken semantic heading hierarchy."
    ),
    capabilities=types.SubagentCapabilities(
        agent_behavior=types.AgentBehavior.AUTONOMOUS,
    ),
)


# ─────────────────────────────────────────────────────────────────────────────
# 4. Root Orchestrator Configuration
# ─────────────────────────────────────────────────────────────────────────────

ORCHESTRATOR_INSTRUCTIONS = """\
You are the VALOR Multi-Agent Code Auditor orchestrator.

Your job:
1. Use the file discovery tools to find all source files in each domain.
2. Delegate each domain to its specialist subagent:
   - hardware_auditor: all *.v/*.sv files
   - systems_auditor: all *.c/*.h files
   - backend_auditor: all *.py files
   - frontend_auditor: all *.html files
3. For each subagent, provide the file list AND tell the subagent to use
   read_source_file to read each file's contents before auditing.
4. Collect the results from all four subagents.
5. Synthesize a final markdown report with:
   - A summary table of issues by domain and severity
   - The single most critical structural risk per layer
   - All individual issues grouped by domain

Be thorough. Read every file. Do not skip files.

IMPORTANT CONTEXT about this project:
- This is the SIH26181 Personal Health Companion & Edge Disaster Monitor
- Target hardware: ESP32-S3 MCU + Renesas ForgeFPGA on Vicharak ShrikeFi board
- The ESP32-S3 only has a SINGLE-PRECISION FPU — flag any use of 'double' in firmware
- The device may operate in disaster zones with NO internet — flag ALL external CDN dependencies
- The FPGA runs at 50MHz, SPI link is asynchronous — flag any missing CDC synchronizers
- FreeRTOS dual-core: Core 0 = optical acquisition, Core 1 = UI/inference — flag shared state without mutex
"""

root_config = LocalAgentConfig(
    subagents=[hardware_auditor, systems_auditor, backend_auditor, frontend_auditor],
    tools=[
        discover_verilog_files,
        discover_c_files,
        discover_python_files,
        discover_html_files,
        read_source_file,
    ],
    capabilities=types.CapabilitiesConfig(
        enable_subagents=True,
        max_subagent_depth=2,
        allowed_subagents=[
            "hardware_auditor",
            "systems_auditor",
            "backend_auditor",
            "frontend_auditor",
        ],
    ),
    system_instructions=ORCHESTRATOR_INSTRUCTIONS,
)


# ─────────────────────────────────────────────────────────────────────────────
# 5. Execution
# ─────────────────────────────────────────────────────────────────────────────

async def run_audit():
    """Run the full multi-agent audit and save results."""
    print("=" * 70)
    print("  VALOR Multi-Agent Code Auditor")
    print(f"  {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    # Verify API key
    api_key = os.environ.get("GEMINI_API_KEY")
    if not api_key:
        print("\nERROR: GEMINI_API_KEY not found in environment.")
        print("Get one free at: https://aistudio.google.com/app/api-keys")
        print("Then: set GEMINI_API_KEY=your_key_here")
        sys.exit(1)

    print("\n[1/3] Starting orchestrator agent...")
    async with Agent(root_config) as agent:
        print("[2/3] Dispatching subagents to audit all domains...\n")

        response = await agent.chat(
            "Run a complete audit of the entire codebase. "
            "Discover files in each domain, delegate to the specialist subagents, "
            "and produce a comprehensive final report in markdown format."
        )

        # Stream output as it arrives
        full_response = ""
        async for chunk in response:
            print(chunk, end="", flush=True)
            full_response += chunk

    # Save report
    report_dir = PROJECT_ROOT / "reports"
    report_dir.mkdir(exist_ok=True)
    report_path = report_dir / "antigravity_audit_report.md"

    header = (
        f"# VALOR Antigravity Multi-Agent Audit Report\n\n"
        f"**Generated:** {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
        f"**Tool:** Google Antigravity SDK (Multi-Agent Orchestration)\n\n"
        f"---\n\n"
    )
    report_path.write_text(header + full_response, encoding="utf-8")

    print(f"\n\n{'=' * 70}")
    print(f"  Report saved to: {report_path}")
    print(f"{'=' * 70}")


if __name__ == "__main__":
    asyncio.run(run_audit())
