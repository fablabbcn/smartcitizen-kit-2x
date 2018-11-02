echo -D__GIT_HASH__=\\\"$(git rev-parse --short --verify master)\\\"
echo -D__ISO_DATE__=\\\"$(date +%Y-%m-%dT%H:%M:%SZ)\\\"
