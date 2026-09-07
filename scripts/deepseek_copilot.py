#!/usr/bin/env python3
"""
DeepSeek AI Copilot for Verilog & Firmware Development
======================================================
Custom tool for SIH26181 Hardware & Firmware Project.
Powered by DeepSeek-V4 (deepseek-v4-pro, deepseek-v4-flash) and DeepSeek-R1 (deepseek-reasoner).

Usage:
  python scripts/deepseek_copilot.py models
  python scripts/deepseek_copilot.py analyze [--pro | --flash | --reason]
  python scripts/deepseek_copilot.py ask "question" [--pro | --flash | --reason]
  python scripts/deepseek_copilot.py audit <file> [--pro | --flash | --reason]
  python scripts/deepseek_copilot.py testbench <module.v> [-o tb_out.v]
  python scripts/deepseek_copilot.py driver <module.v> [-o driver.h]
  python scripts/deepseek_copilot.py sim-debug <logfile.log>
"""

import sys
import os
import json
import argparse
import urllib.request
import urllib.error

API_URL = "https://api.deepseek.com/chat/completions"
MODELS_URL = "https://api.deepseek.com/models"

def load_config():
    """Load API key and default model from environment or .env file."""
    api_key = os.environ.get("DEEPSEEK_API_KEY")
    model = os.environ.get("DEEPSEEK_MODEL")

    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    env_file = os.path.join(root_dir, ".env")
    if os.path.exists(env_file):
        with open(env_file, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not api_key and line.startswith("DEEPSEEK_API_KEY="):
                    api_key = line.split("=", 1)[1].strip().strip('"').strip("'")
                if not model and line.startswith("DEEPSEEK_MODEL="):
                    model = line.split("=", 1)[1].strip().strip('"').strip("'")

    if not model:
        model = "deepseek-v4-pro"

    return api_key, model

def resolve_model(args, default_fallback="deepseek-v4-pro"):
    """Resolve which model to use based on CLI flags and config."""
    if getattr(args, "pro", False):
        return "deepseek-v4-pro"
    if getattr(args, "flash", False):
        return "deepseek-v4-flash"
    if getattr(args, "reason", False):
        return "deepseek-reasoner"
    if getattr(args, "model", None):
        return args.model

    _, config_model = load_config()
    return config_model or default_fallback

def call_deepseek(messages, model=None, api_key=None, stream=True):
    """Send request to DeepSeek API using standard library urllib."""
    config_key, config_model = load_config()
    if not api_key:
        api_key = config_key
    if not model:
        model = config_model

    if not api_key:
        print("\n[ERROR] DeepSeek API Key not found!", file=sys.stderr)
        print("Please set it in your environment or check .env file.\n", file=sys.stderr)
        sys.exit(1)

    payload = {
        "model": model,
        "messages": messages,
        "stream": stream
    }

    req_data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        API_URL,
        data=req_data,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {api_key}"
        }
    )

    try:
        if not stream:
            with urllib.request.urlopen(req) as resp:
                res_json = json.loads(resp.read().decode("utf-8"))
                choice = res_json["choices"][0]["message"]
                if "reasoning_content" in choice and choice["reasoning_content"]:
                    print("\n=== [DeepSeek Reasoning Process] ===")
                    print(choice["reasoning_content"])
                    print("====================================\n")
                return choice.get("content", "")
        else:
            full_content = []
            in_reasoning = False

            with urllib.request.urlopen(req) as resp:
                for line in resp:
                    line_str = line.decode("utf-8").strip()
                    if not line_str or line_str == "data: [DONE]":
                        continue
                    if line_str.startswith("data: "):
                        chunk_json = line_str[6:]
                        try:
                            chunk = json.loads(chunk_json)
                            delta = chunk["choices"][0]["delta"]

                            # Handle reasoning stream
                            if "reasoning_content" in delta and delta["reasoning_content"]:
                                if not in_reasoning:
                                    print("\n=== [DeepSeek Reasoning] ===", flush=True)
                                    in_reasoning = True
                                sys.stdout.write(delta["reasoning_content"])
                                sys.stdout.flush()

                            # Handle actual answer content
                            if "content" in delta and delta["content"]:
                                if in_reasoning:
                                    print("\n=== [Response] ===\n", flush=True)
                                    in_reasoning = False
                                sys.stdout.write(delta["content"])
                                sys.stdout.flush()
                                full_content.append(delta["content"])
                        except Exception:
                            pass
            print()
            return "".join(full_content)

    except urllib.error.HTTPError as e:
        error_body = e.read().decode("utf-8", errors="ignore")
        print(f"\n[HTTP Error {e.code}]: {e.reason}", file=sys.stderr)
        print(error_body, file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"\n[Network Error]: {e}", file=sys.stderr)
        sys.exit(1)

def cmd_models(args):
    """List all available models on the active API key and show current default."""
    api_key, default_model = load_config()
    if not api_key:
        print("[Error] No API key found. Set DEEPSEEK_API_KEY.", file=sys.stderr)
        sys.exit(1)

    req = urllib.request.Request(
        MODELS_URL,
        headers={"Authorization": f"Bearer {api_key}"}
    )
    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            print("\n===========================================================")
            print("         DEEPSEEK MODELS AVAILABLE ON YOUR ACCOUNT         ")
            print("===========================================================")
            for m in data.get("data", []):
                mid = m.get("id")
                tag = " (CURRENT DEFAULT)" if mid == default_model else ""
                desc = ""
                if "v4-pro" in mid:
                    desc = "-> Flagship 1.6T MoE, 1M context, frontier coding & reasoning"
                elif "v4-flash" in mid:
                    desc = "-> High-speed 284B MoE, 1M context, ultra-fast & low cost"
                elif "vision" in mid:
                    desc = "-> Multimodal vision understanding model"
                elif "reasoner" in mid:
                    desc = "-> DeepSeek-R1 pure chain-of-thought reasoning"
                print(f" * {mid:<30} {tag}")
                if desc:
                    print(f"   {desc}")
            print("===========================================================")
            print(f"Current Default Model: {default_model}")
            print("To switch default, add DEEPSEEK_MODEL=<name> to your .env file")
            print("Or pass flags on the fly: --pro, --flash, --reason, or --model <name>\n")
    except Exception as e:
        print("[Error listing models]:", e, file=sys.stderr)

def cmd_analyze(args):
    """Scan and analyze key hardware and firmware files across the project."""
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    
    key_files = [
        "hardware/common/moving_average_8tap.v",
        "hardware/common/ppg_peak_detector.v",
        "hardware/shrikefi/forgefpga_ppg_top.v",
        "hardware/zynq/axi_ppg_accelerator.v",
        "firmware/core/nn_risk_model_int8.c",
        "firmware/core/disaster_risk_engine.c",
        "firmware/shrikefi/main_shrikefi.c"
    ]

    project_context = []
    total_bytes = 0

    for rel_path in key_files:
        full_p = os.path.join(root_dir, rel_path.replace("/", os.sep))
        if os.path.exists(full_p):
            with open(full_p, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
            project_context.append(f"### FILE: {rel_path}\n```\n{content}\n```\n")
            total_bytes += len(content)

    prompt_context = "\n".join(project_context)
    user_instruction = (
        "Here are the core Verilog RTL hardware and embedded firmware files from the SIH26181 project:\n\n"
        f"{prompt_context}\n\n"
        "Please provide a comprehensive technical analysis of this project covering:\n"
        "1. Architecture Overview: How the FPGA hardware accelerator and MCU firmware collaborate.\n"
        "2. Hardware Strengths & Potential Bottlenecks (timing, CDC, filter response, FSM state safety).\n"
        "3. Firmware & TinyML Evaluation (fixed-point precision, ISR latency, disaster risk logic).\n"
        "4. Actionable recommendations for production hardening and verification."
    )

    model = resolve_model(args, "deepseek-v4-pro")
    print(f"[*] Packaging {len(project_context)} project files (~{total_bytes:,} bytes)...")
    print(f"[*] Sending full-codebase analysis to DeepSeek [{model}] (1M context window)...")
    
    messages = [
        {
            "role": "system", 
            "content": "You are a Principal ASIC/FPGA Architect and Embedded Systems Engineer. Provide a rigorous, highly technical analysis of this heterogeneous SoC design."
        },
        {"role": "user", "content": user_instruction}
    ]
    call_deepseek(messages, model=model)

def cmd_audit(args):
    """Audit Verilog or C source code for timing, CDC, race conditions, and bugs."""
    if not os.path.exists(args.file):
        print(f"[Error] File not found: {args.file}", file=sys.stderr)
        sys.exit(1)

    with open(args.file, "r", encoding="utf-8", errors="ignore") as f:
        code = f.read()

    ext = os.path.splitext(args.file)[1].lower()
    is_verilog = ext in [".v", ".sv", ".vh"]

    if is_verilog:
        system_prompt = (
            "You are a Principal ASIC/FPGA Verification & RTL Architect. "
            "Audit the provided synthesizable Verilog module for: "
            "1. Latch inference and incomplete sensitivity lists / case statements. "
            "2. Clock Domain Crossing (CDC) or asynchronous reset pitfalls. "
            "3. Race conditions between blocking (=) and non-blocking (<=) assignments. "
            "4. Timing bottleneck risks (long combinational paths, unpipelined arithmetic). "
            "5. Hardware resource waste (DSP48/multiplier, BRAM, excessive LUT usage). "
            "Provide actionable, line-specific fixes with replacement Verilog code."
        )
    else:
        system_prompt = (
            "You are a Principal Embedded Systems and Firmware Engineer specializing in "
            "ESP-IDF, FreeRTOS, and ARM bare-metal/HAL drivers. "
            "Audit the provided C source code for: "
            "1. Memory safety, buffer overruns, unaligned access. "
            "2. Interrupt Service Routine (ISR) safety and race conditions. "
            "3. Concurrency / FreeRTOS task starvation or deadlock risks. "
            "4. Precision loss or overflow in sensor math / fixed-point calculations. "
            "Provide line-specific fixes with clean, idiomatic C code."
        )

    user_prompt = f"Target File: {args.file}\n\n```{ext[1:] if ext else 'text'}\n{code}\n```"
    model = resolve_model(args, "deepseek-v4-pro")
    print(f"[*] Running DeepSeek audit using [{model}] on {args.file}...")
    messages = [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": user_prompt}
    ]
    call_deepseek(messages, model=model)

def cmd_testbench(args):
    """Generate a self-checking Verilog testbench for an RTL module."""
    if not os.path.exists(args.file):
        print(f"[Error] File not found: {args.file}", file=sys.stderr)
        sys.exit(1)

    with open(args.file, "r", encoding="utf-8", errors="ignore") as f:
        code = f.read()

    system_prompt = (
        "You are an expert digital design verification engineer. "
        "Generate a complete, cycle-accurate, self-checking Verilog testbench (tb_*.v) for the given module. "
        "Requirements:\n"
        "1. Standard clock generator (50 MHz default, period 20ns) and power-on reset sequence.\n"
        "2. Comprehensive stimulus vectors covering typical operation, reset recovery, and corner cases.\n"
        "3. Automated assertions / self-checking logic with $display and $fatal for pass/fail tally.\n"
        "4. VCD dump variables enabled ($dumpfile, $dumpvars) for GTKWave waveform visualization.\n"
        "5. Clean, fully synthesizable Verilog-2001 compatible syntax (compiles with iverilog)."
    )

    user_prompt = f"Module File: {args.file}\n\n```verilog\n{code}\n```\nGenerate the complete self-checking testbench."
    model = resolve_model(args, "deepseek-v4-pro")
    print(f"[*] Generating self-checking testbench using [{model}] for {args.file}...")
    messages = [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": user_prompt}
    ]
    res = call_deepseek(messages, model=model, stream=not bool(args.output))

    if args.output:
        output_text = res
        if "```verilog" in output_text:
            output_text = output_text.split("```verilog")[1].split("```")[0].strip()
        elif "```" in output_text:
            output_text = output_text.split("```")[1].split("```")[0].strip()

        with open(args.output, "w", encoding="utf-8") as f:
            f.write(output_text + "\n")
        print(f"\n[SUCCESS] Testbench written to {args.output}")

def cmd_driver(args):
    """Generate C firmware drivers and register maps from a Verilog RTL module."""
    if not os.path.exists(args.file):
        print(f"[Error] File not found: {args.file}", file=sys.stderr)
        sys.exit(1)

    with open(args.file, "r", encoding="utf-8", errors="ignore") as f:
        code = f.read()

    system_prompt = (
        "You are an embedded software architect. "
        "Given the Verilog RTL module with memory-mapped registers, control signals, or bus interfaces, "
        "generate a production-quality C header (.h) and driver implementation for ESP32-S3 / ARM Cortex-M/A. "
        "Include:\n"
        "1. Precise register address offset definitions (#define REG_... 0x...).\n"
        "2. Bitfield shift and mask macros for status, configuration, and interrupt registers.\n"
        "3. Strongly-typed C struct overlays and volatile pointer accessors.\n"
        "4. High-level initialization, sample read, and status clear (W1C) driver functions."
    )

    user_prompt = f"Verilog Module: {args.file}\n\n```verilog\n{code}\n```\nGenerate the C driver header and implementation."
    model = resolve_model(args, "deepseek-v4-flash")
    print(f"[*] Generating C driver using [{model}] for {args.file}...")
    messages = [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": user_prompt}
    ]
    res = call_deepseek(messages, model=model, stream=not bool(args.output))

    if args.output:
        output_text = res
        if "```c" in output_text:
            output_text = output_text.split("```c")[1].split("```")[0].strip()
        elif "```" in output_text:
            output_text = output_text.split("```")[1].split("```")[0].strip()

        with open(args.output, "w", encoding="utf-8") as f:
            f.write(output_text + "\n")
        print(f"\n[SUCCESS] Driver written to {args.output}")

def cmd_sim_debug(args):
    """Diagnose simulation failures or compilation logs."""
    if not os.path.exists(args.file):
        print(f"[Error] Log file not found: {args.file}", file=sys.stderr)
        sys.exit(1)

    with open(args.file, "r", encoding="utf-8", errors="ignore") as f:
        log_content = f.read()

    system_prompt = (
        "You are an EDA and embedded build engineer. Analyze the following compilation or simulation log "
        "(from iverilog, vvp, Vivado, GCC, or ESP-IDF). Identify:\n"
        "1. The exact root cause of the error or warning.\n"
        "2. Which specific file and line is triggering the issue.\n"
        "3. Concrete step-by-step fix commands or code patches to resolve the error."
    )

    user_prompt = f"Build / Simulation Log: {args.file}\n\n```text\n{log_content}\n```"
    model = resolve_model(args, "deepseek-v4-pro")
    print(f"[*] Diagnosing error log using [{model}]...")
    messages = [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": user_prompt}
    ]
    call_deepseek(messages, model=model)

def cmd_ask(args):
    """Ask a free-form question to DeepSeek."""
    model = resolve_model(args, "deepseek-v4-pro")
    system_prompt = (
        "You are an expert pair-programming AI assistant specialized in digital design (Verilog/SystemVerilog), "
        "FPGA acceleration, TinyML edge inference, and embedded C firmware for the SIH26181 Health Companion project."
    )
    print(f"[*] Querying [{model}]...")
    messages = [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": args.prompt}
    ]
    call_deepseek(messages, model=model)

def add_model_args(parser):
    """Add standard model selection flags to a subparser."""
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--pro", "-p", action="store_true", help="Use deepseek-v4-pro (Flagship 1.6T MoE)")
    group.add_argument("--flash", "-f", action="store_true", help="Use deepseek-v4-flash (Fast & low cost)")
    group.add_argument("--reason", "-r", action="store_true", help="Use deepseek-reasoner (R1 reasoning)")
    group.add_argument("--model", "-m", help="Specify exact model name")

def main():
    # If the user omitted the subcommand keyword (e.g. .\deepseek.bat "Hello" --flash),
    # automatically insert 'ask' so they never get an 'invalid choice' error!
    subcommands = ["models", "analyze", "audit", "testbench", "driver", "sim-debug", "ask", "-h", "--help"]
    if len(sys.argv) > 1 and sys.argv[1] not in subcommands:
        sys.argv.insert(1, "ask")

    parser = argparse.ArgumentParser(description="DeepSeek Copilot for Verilog & Firmware")
    subparsers = parser.add_subparsers(dest="command", required=True)

    # models
    p_models = subparsers.add_parser("models", help="List all available models and current active default")
    p_models.set_defaults(func=cmd_models)

    # analyze
    p_analyze = subparsers.add_parser("analyze", help="Read and analyze all key files in hardware and firmware folders")
    add_model_args(p_analyze)
    p_analyze.set_defaults(func=cmd_analyze)

    # audit
    p_audit = subparsers.add_parser("audit", help="Run deep code & safety audit on Verilog or C files")
    p_audit.add_argument("file", help="Path to .v or .c source file")
    add_model_args(p_audit)
    p_audit.set_defaults(func=cmd_audit)

    # testbench
    p_tb = subparsers.add_parser("testbench", help="Generate a self-checking testbench for an RTL module")
    p_tb.add_argument("file", help="Path to synthesizable .v module")
    p_tb.add_argument("-o", "--output", help="Save testbench to specified file")
    add_model_args(p_tb)
    p_tb.set_defaults(func=cmd_testbench)

    # driver
    p_drv = subparsers.add_parser("driver", help="Generate C firmware drivers from a Verilog register map")
    p_drv.add_argument("file", help="Path to Verilog module with registers")
    p_drv.add_argument("-o", "--output", help="Save driver header to specified file")
    add_model_args(p_drv)
    p_drv.set_defaults(func=cmd_driver)

    # sim-debug
    p_dbg = subparsers.add_parser("sim-debug", help="Diagnose simulation or compilation error logs")
    p_dbg.add_argument("file", help="Path to log file")
    add_model_args(p_dbg)
    p_dbg.set_defaults(func=cmd_sim_debug)

    # ask
    p_ask = subparsers.add_parser("ask", help="Ask DeepSeek any engineering question")
    p_ask.add_argument("prompt", help="Question or instructions")
    add_model_args(p_ask)
    p_ask.set_defaults(func=cmd_ask)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
