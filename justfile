set windows-shell := ["cmd.exe", "/c"]

# Update currentGitHash.txt and currentGit.h from HEAD
[windows]
hash:
    updateGitHash.bat

[unix]
hash:
    ./updateGitHash.command

# Bump version across all hi_xxx module headers + .jucer files, then create git tag vX.Y.Z
bump VERSION:
    python tools/bump_version.py {{VERSION}}
    git add -A "hi_*/hi_*.h" "projects/plugin/HISE.jucer" "projects/standalone/HISE Standalone.jucer"
    git commit -m "bump version to {{VERSION}}"
    git tag "v{{VERSION}}"
