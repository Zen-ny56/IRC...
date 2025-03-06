if git status | grep -w "modified" > /dev/null; then
    if ls -la | grep -w ".vscode" > /dev/null; then
        rm -f .vscode
    fi
    git add .; git commit -m "Updated"; git push;
fi