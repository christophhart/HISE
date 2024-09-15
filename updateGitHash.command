cd "$(dirname "$0")"

commit=$(git rev-parse HEAD)
line="#define PREVIOUS_HISE_COMMIT \"$commit\""

echo "$line" > hi_backend/backend/currentGit.h
echo $commit > currentGitHash.txt
