# GitHub Best Practices: A Developer's Field Guide

> A practical, no-nonsense guide to commits, branching, versioning, and automation.
> Written for developers who know how to code but want to feel confident about their workflow.

---

## Table of Contents

1. [The Philosophy Behind Good Git Usage](#1-the-philosophy-behind-good-git-usage)
2. [Commits: What They Are and How to Use Them](#2-commits-what-they-are-and-how-to-use-them)
3. [Versioning: Releases vs. Commits](#3-versioning-releases-vs-commits)
4. [Branching Strategy](#4-branching-strategy)
5. [Pull Requests](#5-pull-requests)
6. [Automation: What to Automate and What Not To](#6-automation-what-to-automate-and-what-not-to)
7. [The .gitignore and Repository Hygiene](#7-the-gitignore-and-repository-hygiene)
8. [README and Documentation Habits](#8-readme-and-documentation-habits)
9. [Daily Developer Habits](#9-daily-developer-habits)
10. [Red Flags to Watch For](#10-red-flags-to-watch-for)
11. [Quick Reference Cheat Sheet](#11-quick-reference-cheat-sheet)

---

## 1. The Philosophy Behind Good Git Usage

Git is not a backup tool. It is a **communication tool** — you are leaving a trail of decisions for your future self, your collaborators, and anyone who reads the history of your project.

Every commit, branch name, and tag is a message. The question to ask before any Git action is:

> *"If someone reads only this, will they understand what changed and why?"*

**The two failure modes:**
- **Too vague:** `fix stuff`, `update`, `changes` — tells no one anything
- **Too granular:** committing every line change separately — creates noise, not signal

The goal is a history that reads like a changelog: clean, chronological, purposeful.

---

## 2. Commits: What They Are and How to Use Them

### What a commit represents

A commit should represent **one logical unit of work** — not one file, not one function, not one hour. A logical unit is a change that could be described in a single sentence and that leaves the codebase in a working (or intentionally broken-and-marked) state.

**Examples of good commit boundaries:**
- Adding a new shell command end-to-end (header, implementation, command table entry)
- Fixing a specific bug
- Refactoring one module (no behavior change)
- Updating documentation for a feature

**Examples of bad commit boundaries:**
- "Everything I did today"
- Saving your work mid-feature because you're afraid of losing it (use a branch or `git stash` instead)
- One commit per file when the files are part of the same change

---

### Commit message format

The most widely adopted standard is the **Conventional Commits** format. It is simple and machine-readable:

```
<type>(<optional scope>): <short summary>

<optional body — the WHY, not the WHAT>

<optional footer: BREAKING CHANGE, closes #issue>
```

**Types:**

| Type       | When to use |
|------------|-------------|
| `feat`     | A new feature or command |
| `fix`      | A bug fix |
| `refactor` | Code restructure with no behavior change |
| `docs`     | Documentation only |
| `chore`    | Build system, tooling, config (no production code) |
| `test`     | Adding or fixing tests |
| `perf`     | Performance improvement |
| `revert`   | Reverting a previous commit |

**Rules for the summary line:**
- Imperative mood: "Add matrix command" not "Added" or "Adding"
- No period at the end
- 72 characters max
- Lowercase after the colon

**Examples:**

```
feat(shell): add matrix command with configurable speed

fix(ata): handle timeout when disk is not present

refactor(gui): extract widget drawing into separate functions

docs: update README with build instructions for macOS
```

---

### The body: the most underused part

The body is where you explain **why** the change was made, not what. The diff already shows what changed.

```
feat(persist): save aliases on every alias command

Previously aliases were only saved on explicit `save` calls,
which meant a crash would lose all session aliases. Auto-saving
on every write ensures no data is lost without requiring the
user to remember to save.
```

Use the body when:
- The reason for the change is not obvious
- You made a deliberate trade-off worth documenting
- You worked around a bug or limitation

Skip the body when:
- The summary line is self-explanatory (`docs: fix typo in README`)

---

### Atomic commits in practice

Before committing, ask: *"Can I describe this in one sentence without using 'and'?"*

If you say "Add the matrix command **and** fix the cursor bug" — that is two commits.

Use `git add -p` (patch mode) to stage only parts of a file when you accidentally mixed two changes together:

```bash
git add -p          # interactively stage hunks
git diff --staged   # review what you are about to commit
git commit
```

---

## 3. Versioning: Releases vs. Commits

This is one of the most common points of confusion, and it is worth being direct about.

### The core distinction

| Concept | Lives in | Purpose |
|---------|----------|---------|
| **Commit message** | Every commit | Describe one logical change |
| **Version number** | Git tags + release notes | Mark a meaningful milestone |

Version numbers belong on **tags**, not in commit messages.

### What you are likely doing now

If your commits look like:
```
Add matrix command (BETA) v2.3.0
Implement persistence (basic) v2.1.1
```

The version embedded in the message will drift out of sync, become hard to search, and conflates two separate concepts. Someone reading your history cannot tell which commit *is* v2.1.1 without reading every message.

### The correct approach: Git tags

After finishing a meaningful milestone, tag it:

```bash
git tag -a v2.3.0 -m "Add matrix command (BETA)"
git push origin v2.3.0
```

Then your commits describe individual changes and your tags mark releases:

```
git log --oneline:
  ac40857  feat(shell): add matrix command with configurable speed
  61080a3  feat(fs): introduce savelist concept
  e4c00fc  feat(persist): implement ATA-backed persistence

git tag:
  v2.3.0  →  ac40857
  v2.2.1  →  61080a3
```

### Semantic Versioning (SemVer)

The standard versioning scheme is `MAJOR.MINOR.PATCH`:

| Part    | Bump when |
|---------|-----------|
| `MAJOR` | Breaking change — something that was working will now break |
| `MINOR` | New feature, backwards compatible |
| `PATCH` | Bug fix, no new features |

For a personal OS project in active development, `0.x.y` is appropriate until you consider it stable enough for a 1.0.0 release. Using `v2.3.0` implies a stable, mature product — use `v0.23.0` or `v0.2.3` if you are still in heavy development.

---

## 4. Branching Strategy

### Why branches exist

Branches are **workspaces**. `main` (or `master`) is the source of truth — it should always be in a working, deployable state. Branches are where experiments and features live until they are ready.

### The simplest strategy that actually works: GitHub Flow

1. `main` is always deployable
2. Create a branch for anything you are working on
3. Open a pull request when you want feedback or want to merge
4. Merge to `main` when done, delete the branch

```
main ─────●────────────────────────●─────────
           \                      /
            feat/keyboard-driver ●
```

### Branch naming

Use a consistent pattern:

```
feat/matrix-command
fix/ata-timeout-bug
refactor/gui-widget-extraction
docs/build-instructions
chore/clean-unused-headers
```

Or with issue numbers when you use GitHub Issues:

```
feat/42-matrix-command
fix/17-ata-timeout
```

### What goes on `main`

Only finished, working code. If you are mid-feature and need to commit to preserve work, commit to your feature branch. Never commit broken code to `main`.

### Protecting `main`

On GitHub, go to Settings → Branches → Add branch protection rule for `main`:
- Require pull request before merging
- Require at least 1 review (even if it is just you reviewing your own diff)

This forces you into a pull request workflow, which builds the habit of reviewing your own work before merging.

---

## 5. Pull Requests

### What a pull request is

A pull request (PR) is a **proposal to merge** — it is a structured conversation around a set of commits. Even on a solo project, PRs are valuable because they force you to write a summary of what you changed and why, and to look at the diff one more time before it hits `main`.

### A good PR description

```markdown
## What
Adds the `matrix` command to the shell. Renders a falling-character animation
in the framebuffer using the VESA driver.

## Why
Demonstrates the framebuffer capabilities and is visually engaging for demos.

## How
- New file: shell/matrix.c (rendering loop, character table)
- shell/commands.c: added `matrix` to the command table
- Configurable speed via argument (default: medium)

## Testing
- Ran in QEMU, no framebuffer corruption observed
- ESC exits cleanly without leaving artifacts
```

### PR size

Smaller PRs are better PRs. A PR that changes 50 lines is easy to review. A PR that changes 2,000 lines will never get a thorough review.

If a feature is large, break it into sequential PRs:
1. PR 1: data structures and types
2. PR 2: implementation
3. PR 3: integration into command table

---

## 6. Automation: What to Automate and What Not To

### Good automation targets

| Automate | Because |
|----------|---------|
| Linting / formatting | Enforces consistency without opinion |
| Build verification | Catches broken builds immediately |
| Running tests | Fast feedback loop |
| Tagging releases | Reduces human error in version numbers |

### Bad automation targets

| Do not automate | Because |
|-----------------|---------|
| Writing commit messages | Messages require understanding of intent — automation produces noise |
| Auto-committing on save | Creates meaningless micro-history |
| Auto-pushing every commit | Pushes work-in-progress to the remote constantly |
| Auto-merging PRs | Removes the review step |

### The right model for a commit automation system

If you have a script that auto-stages and commits, that is only helpful if it:
1. Still requires you to write the message manually
2. Only runs when you explicitly invoke it
3. Never commits to `main` directly

A reasonable helper script might:
- Run `git diff --staged` so you can review what you are committing
- Prompt you to type a message following the conventional commits format
- Run the build before committing (so you never commit broken code)

```bash
#!/bin/bash
# commit-helper.sh
set -e

echo "=== Staged changes ==="
git diff --staged --stat

echo ""
read -p "Commit message (type feat/fix/chore/etc): " msg

echo "=== Building ==="
make

git commit -m "$msg"
echo "Committed. Remember to push when ready."
```

### GitHub Actions: a practical starting point

For a C/NASM project, a basic CI workflow that builds and reports pass/fail is valuable:

```yaml
# .github/workflows/build.yml
name: Build

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install cross-compiler
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-i686-linux-gnu nasm

      - name: Build
        run: make
```

This gives you a green/red badge on every PR and catches builds that only break on a clean machine.

---

## 7. The .gitignore and Repository Hygiene

### Always ignore

For a C/NASM project:

```gitignore
# Build artifacts
*.o
*.bin
*.iso
*.img
*.elf

# OS artifacts
.DS_Store
Thumbs.db

# Editor artifacts
*.swp
*.swo
.vscode/
*.iml

# Local config
*.local
```

### Never commit

- Binary build outputs (`.bin`, `.iso`, `.img`) — they are reproducible from source
- Credentials, API keys, tokens
- Your local editor config (`.vscode/settings.json`) unless the team agreed to share it

---

## 8. README and Documentation Habits

### The README is a first impression

Every project should have a README that answers:
1. What is this?
2. How do I build it?
3. How do I run it?
4. What are the key commands / features?

For DahleOS, a good README structure:

```markdown
# DahleOS

A 32-bit x86 bare-metal operating system targeting QEMU.

## Requirements
- i686-elf-gcc cross-compiler
- NASM
- QEMU

## Building
make        # produces os.bin
make iso    # produces bootable ISO
make run    # boots in QEMU

## Shell Commands
...
```

### CHANGELOG

Keep a `CHANGELOG.md` updated at each tagged release. Use this format:

```markdown
# Changelog

## [v2.3.0] - 2026-05-01
### Added
- `matrix` command with configurable speed

## [v2.2.1] - 2026-04-20
### Added
- `savelist` command

### Fixed
- Aliases not persisting after crash
```

---

## 9. Daily Developer Habits

These are habits, not rules. Build them one at a time.

### Before you start coding
```bash
git status          # know what state you are in
git pull            # get latest from remote (on team projects)
git checkout -b feat/your-feature   # start a branch
```

### While coding
- Commit when a logical unit is complete — not on a timer
- Use `git diff` before staging to review what changed
- Use `git add -p` when a file has mixed changes
- Write the commit message before you forget why you made the change

### Before pushing
```bash
git log --oneline -5    # read your own history
git diff main...HEAD    # see everything you are about to push
make                    # build one more time
```

### Weekly
- Look at your recent commits. Do they tell a story?
- Close stale branches
- Tag a release if you hit a milestone

---

## 10. Red Flags to Watch For

These are signs your workflow is drifting:

| Red flag | What it means |
|----------|--------------|
| Commits like `fix`, `update`, `stuff` | You are not thinking about the message |
| Dozens of commits but no tags | No concept of releases |
| `main` has broken commits | No branch-and-merge discipline |
| 3000-line PRs | Feature was not scoped correctly |
| Committing `.bin` or `.img` files | Missing `.gitignore` rules |
| `wip`, `temp`, `test123` branches open for weeks | Branches are being used as parking lots |
| Version numbers only in commit messages, not tags | Version and history are conflated |

---

## 11. Quick Reference Cheat Sheet

### Commit message format
```
feat(scope): short summary in imperative mood

Optional body explaining WHY (not WHAT).
The diff already shows what changed.
```

### Daily commands
```bash
git status
git diff
git diff --staged
git add -p
git log --oneline -10
git stash / git stash pop
```

### Tagging a release
```bash
git tag -a v0.2.3 -m "Brief description of this release"
git push origin v0.2.3
```

### Branch workflow
```bash
git checkout -b feat/my-feature     # start
# ... work ...
git push -u origin feat/my-feature  # push
# open PR on GitHub
# review and merge
git checkout main && git pull        # sync
git branch -d feat/my-feature        # clean up
```

### Check what is different from main
```bash
git diff main...HEAD        # all changes since branching
git log main..HEAD          # all commits since branching
```

---

## Summary

The three changes that will make the biggest difference immediately:

1. **Move version numbers to tags, not commit messages.** Use `git tag -a v0.x.y` after a milestone.

2. **Write commit messages that answer "why" in the body.** The summary says what; the body says why.

3. **Evaluate every automation tool by asking: does it replace my thinking or amplify it?** Automation should enforce consistency, never replace intent.

Git history is a professional artifact. Treat it like the documentation layer above your code — because that is exactly what it is.
