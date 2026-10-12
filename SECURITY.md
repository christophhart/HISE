# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in HISE, please **do not** open a public GitHub issue.

Instead, report it responsibly by emailing:

**security@hise.audio**

Please include:
- A clear description of the vulnerability
- Steps to reproduce
- Potential impact
- Any suggested fix (optional)

## What to Expect

- **Acknowledgement**: Within 48 hours of your report
- **Status update**: Within 7 days with an initial assessment
- **Resolution**: Critical issues within 14 days, others within 30 days
- **Credit**: In the release notes (if desired) once the issue is resolved

## Scope

### In Scope

- HISE core framework (`hi_core/`)
- Backend components (`hi_backend/`)
- DSP library (`hi_dsp_library/`)
- Scripting engine (HiseScript)
- Plugin export functionality (VST/AU/AAX)
- Sample loading and playback
- Network-related features

### Out of Scope

- Third-party dependencies (report to their maintainers)
- Social engineering attacks
- Denial of service attacks
- Issues in deprecated or archived code
- User-created HISE scripts and instruments

## Security Considerations

HISE is an audio application that processes:
- Audio files (WAV, AIFF, FLAC, etc.)
- Sample libraries
- User scripts (HiseScript)
- Preset files

When reporting vulnerabilities, consider:
- **Audio processing**: Buffer overflows, integer overflows in DSP code
- **Script execution**: Sandbox escapes, code injection
- **File parsing**: Malformed sample files, preset injection
- **Network**: Update mechanism, license validation

## Dependencies

HISE uses several third-party libraries including:
- JUCE framework
- FFTW
- Various audio codecs

We recommend keeping dependencies updated and monitoring for known vulnerabilities.

## Security Updates

Security patches are released as soon as possible. Subscribe to our [GitHub Security Advisories](https://github.com/christophhart/HISE/security/advisories) to stay informed.

## Contact

For security-related inquiries: **security@hise.audio**
