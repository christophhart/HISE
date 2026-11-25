#!/bin/bash
set -euo pipefail

fail() {
	echo "Signing preflight failed: $1" >&2
	exit 1
}

: "${APPLE_APPLICATION_SIGNING_IDENTITY:?APPLE_APPLICATION_SIGNING_IDENTITY secret is missing}"
: "${APPLE_INSTALLER_SIGNING_IDENTITY:?APPLE_INSTALLER_SIGNING_IDENTITY secret is missing}"
: "${APPLE_NOTARY_PROFILE:?APPLE_NOTARY_PROFILE secret is missing}"

identities="$(security find-identity -v)" || fail "Could not read signing identities from the runner keychain."

if ! grep -Fq -- "$APPLE_APPLICATION_SIGNING_IDENTITY" <<< "$identities"; then
	fail "The application signing identity is not available in the runner keychain."
fi

if ! grep -Fq -- "$APPLE_INSTALLER_SIGNING_IDENTITY" <<< "$identities"; then
	fail "The installer signing identity is not available in the runner keychain."
fi

if ! xcrun notarytool history --keychain-profile "$APPLE_NOTARY_PROFILE" >/dev/null 2>&1; then
	fail "The notarytool keychain profile is missing or unusable."
fi

echo "Signing identities and notarytool profile are available."
