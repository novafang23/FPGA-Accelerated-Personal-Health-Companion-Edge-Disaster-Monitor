# Project Roadmap

Short, practical roadmap for the SIH26181 Health Companion project — focused on making the codebase reliable and easy to work with.

## Phase 1 — Audit & Hygiene (completed / in progress)
- CI: added GitHub Actions to build the C code, run unit tests, and lint Python (see .github/workflows/ci.yml and ci-extended.yml).
- Static analysis: added cppcheck and verilator steps and basic linting configs.
- Engine hardening: moved magic numbers into named macros and added input validation in SIH/disaster_risk_engine.c.
- Unit tests: basic test harness for the disaster risk logic (SIH/tests/test_disaster_risk.c).
- Issues: opened prioritized follow-ups for CI, linters, tests, secrets, and docs.

## Short-term backlog
1. Turn linting and static checks into blocking CI checks.
2. Run clang-tidy/cppcheck across the C sources and address the top findings.
3. Finish SIH hardening: centralize configuration into a header and document thresholds.
4. Add secret scanning to CI and remediate any findings.
5. Improve README and add CONTRIBUTING notes for local builds and tests.

## Longer-term
- Porting to Snapdragon/Hexagon (performance work) and field validation (hardware-in-the-loop, thermal tests).

Next step
- Merge this PR once you’re ready; follow-up work can be split into focused PRs for clarity.