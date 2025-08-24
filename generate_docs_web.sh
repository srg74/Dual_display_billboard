#!/bin/bash
# ESP32 Dual Display Billboard - Documentation Generator
# This script pushes to GitHub to trigger automatic documentation generation

echo " ESP32 Dual Display Billboard - Documentation Generator"
echo "========================================================="
echo ""

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo " ERROR: Not in a git repository!"
    exit 1
fi

# Check current branch
CURRENT_BRANCH=$(git branch --show-current)
echo " Current branch: $CURRENT_BRANCH"

# Check if there are changes to commit
if ! git diff-index --quiet HEAD --; then
    echo "  You have uncommitted changes!"
    echo ""
    git status --short
    echo ""
    echo "Please commit your changes first:"
    echo "  git add ."
    echo "  git commit -m \"Update documentation\""
    echo "  ./generate_docs_web.sh"
    exit 1
fi

echo " No uncommitted changes found"

# Push to trigger documentation generation
echo ""
echo " Pushing to GitHub to trigger documentation generation..."

if git push origin "$CURRENT_BRANCH"; then
    echo " Successfully pushed to GitHub!"
else
    echo " Failed to push to GitHub"
    exit 1
fi

echo ""
echo " Documentation generation started!"
echo ""
echo " What happens next:"
echo "1. GitHub Actions will build the documentation"
echo "2. You can download it from GitHub Actions artifacts"
echo "3. If on main branch, it will be published to GitHub Pages"
echo ""
echo " View progress at:"
echo "   https://github.com/srg74/Dual_display_billboard/actions"
echo ""
echo " Tip: Open the documentation portal:"
echo "   open docs/documentation-portal.html"