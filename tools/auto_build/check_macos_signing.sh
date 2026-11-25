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

# The installer submission checks the notary profile directly. A history request
# can fail even when submission works, so do not use it as a signing preflight.
echo "Signing identities are available; the notary profile will be checked during submission."
