set windows-shell := ["cmd.exe", "/c"]

# Update currentGitHash.txt and currentGit.h from HEAD
[windows]
hash:
    updateGitHash.bat

[unix]
hash:
    ./updateGitHash.command

# Bump version across all hi_xxx module headers + .jucer files, resave Projucer projects, then create git tag vX.Y.Z
[windows]
bump VERSION:
    updateGitHash.bat
    python tools/bump_version.py bump {{VERSION}}
    python tools/bump_version.py resave
    python tools/bump_version.py finalize {{VERSION}}

[unix]
bump VERSION:
    ./updateGitHash.command
    python tools/bump_version.py bump {{VERSION}}
    python tools/bump_version.py resave
    python tools/bump_version.py finalize {{VERSION}}

[windows]
precompiled-binaries:
    cd tools\auto_build && call 01_BuildHISE.bat && call 02_BuildInstaller.bat

[unix]
precompiled-binaries:
    bash tools/auto_build/01_BuildHISE.sh && bash tools/auto_build/02_BuildInstaller.sh

# Dump live REST API OpenAPI spec from running HISE backend (port 1900) into guidelines/api/openapi.json
dump-openapi:
    curl -s http://127.0.0.1:1900/ | python -m json.tool > guidelines/api/openapi.json
