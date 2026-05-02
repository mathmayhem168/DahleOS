#!/bin/bash
# =============================================================
#  DahleOS commit helper
#  Usage: ./tools/commit.sh
#
#  Enforces conventional commits format, builds before
#  committing, and optionally tags a release.
# =============================================================

set -e

# ── Colors ────────────────────────────────────────────────────
RED='\033[0;31m'
GRN='\033[0;32m'
YLW='\033[1;33m'
BLU='\033[0;34m'
NC='\033[0m'

# ── Check: is there anything staged? ─────────────────────────
if git diff --staged --quiet; then
    echo -e "${RED}Nothing staged. Run 'git add' first.${NC}"
    exit 1
fi

# ── Show what is staged ───────────────────────────────────────
echo -e "${BLU}=== Staged changes ===${NC}"
git diff --staged --stat
echo ""

# ── Pick commit type ──────────────────────────────────────────
echo -e "${YLW}Commit type:${NC}"
echo "  1) feat     – new feature or command"
echo "  2) fix      – bug fix"
echo "  3) refactor – code restructure, no behavior change"
echo "  4) docs     – documentation only"
echo "  5) chore    – build system, tooling, config"
echo "  6) perf     – performance improvement"
echo "  7) revert   – reverting a previous commit"
echo ""
read -rp "Choose [1-7]: " type_choice

case $type_choice in
    1) TYPE="feat" ;;
    2) TYPE="fix" ;;
    3) TYPE="refactor" ;;
    4) TYPE="docs" ;;
    5) TYPE="chore" ;;
    6) TYPE="perf" ;;
    7) TYPE="revert" ;;
    *) echo -e "${RED}Invalid choice.${NC}"; exit 1 ;;
esac

# ── Optional scope ────────────────────────────────────────────
read -rp "Scope (optional, e.g. shell, ata, gui — press Enter to skip): " SCOPE

# ── Summary line ──────────────────────────────────────────────
echo ""
echo -e "${YLW}Write a short summary (imperative mood, no period):${NC}"
echo "  Good: add matrix command with configurable speed"
echo "  Good: fix ATA timeout when disk is not present"
echo "  Bad:  Added matrix. Fixed stuff."
echo ""
read -rp "Summary: " SUMMARY

if [ -z "$SUMMARY" ]; then
    echo -e "${RED}Summary cannot be empty.${NC}"
    exit 1
fi

# ── Optional body ─────────────────────────────────────────────
echo ""
echo -e "${YLW}Body (optional — explain WHY, not WHAT; press Enter twice to finish):${NC}"
BODY=""
while IFS= read -rp "" line; do
    [ -z "$line" ] && break
    BODY="${BODY}${line}
"
done

# ── Assemble message ──────────────────────────────────────────
if [ -n "$SCOPE" ]; then
    HEADER="${TYPE}(${SCOPE}): ${SUMMARY}"
else
    HEADER="${TYPE}: ${SUMMARY}"
fi

if [ -n "$BODY" ]; then
    FULL_MSG="${HEADER}

${BODY}"
else
    FULL_MSG="${HEADER}"
fi

# ── Preview ───────────────────────────────────────────────────
echo ""
echo -e "${BLU}=== Commit message preview ===${NC}"
echo "$FULL_MSG"
echo ""
read -rp "Looks good? [y/N] " confirm
if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 0
fi

# ── Build check ───────────────────────────────────────────────
echo ""
echo -e "${BLU}=== Building DahleOS ===${NC}"
if ! make --no-print-directory 2>&1; then
    echo ""
    echo -e "${RED}Build failed — commit aborted. Fix errors and try again.${NC}"
    exit 1
fi
echo -e "${GRN}Build OK.${NC}"

# ── Commit ────────────────────────────────────────────────────
git commit -m "$FULL_MSG"
echo ""
echo -e "${GRN}Committed!${NC}"

# ── Optional tag ─────────────────────────────────────────────
echo ""
read -rp "Tag a release? (leave blank to skip, or enter version e.g. v0.2.4): " TAG

if [ -n "$TAG" ]; then
    # Validate format loosely
    if [[ ! "$TAG" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo -e "${YLW}Warning: '$TAG' doesn't match vMAJOR.MINOR.PATCH format.${NC}"
        read -rp "Tag anyway? [y/N] " force_tag
        [[ ! "$force_tag" =~ ^[Yy]$ ]] && echo "Skipping tag." && exit 0
    fi
    read -rp "Tag message (brief release note): " TAG_MSG
    git tag -a "$TAG" -m "$TAG_MSG"
    echo -e "${GRN}Tagged $TAG.${NC}"
    echo ""
    echo -e "${YLW}Remember to push the tag:${NC}  git push origin $TAG"
fi

echo ""
echo -e "${YLW}Remember to push when ready:${NC}  git push"
