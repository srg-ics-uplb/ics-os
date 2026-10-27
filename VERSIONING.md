# Versioning Policy

ICS-OS follows [Semantic Versioning 2.0.0](https://semver.org/) (`MAJOR.MINOR.PATCH`).

The first official release is **v1.0.0**, which serves as the baseline for all later versions.

## Version Format

```
vMAJOR.MINOR.PATCH[-PRERELEASE]
```

Examples: `v1.0.0`, `v1.2.3`, `v2.0.0-rc1`

Git tags always carry a leading `v`.

## What Each Number Means

Because ICS-OS is a kernel used for teaching rather than a library with a traditional API, "public interface" here means the things that instructors, students, and existing labs depend on: the system call interface, the shell and user-program interface, the build environment, and the boot process.

### MAJOR (`v2.0.0`)

Increment when a change breaks compatibility with existing labs, handouts, or user programs. Examples:

- Changes to or removal of system calls or other kernel interfaces
- Changes to the executable format or how user programs are built or loaded
- A new required toolchain or build environment (for example, a different compiler, base image, or OS for building)
- A change to the boot method or target hardware requirements
- Removal of a major feature or lab

When MAJOR is incremented, MINOR and PATCH reset to 0.

### MINOR (`v1.1.0`)

Increment when functionality is added in a backward-compatible way. Examples:

- A new device driver, kernel module, or filesystem feature
- A new shell command or user application
- A new lab or substantial new lab material
- Notable improvements that do not break existing code or labs

When MINOR is incremented, PATCH resets to 0.

### PATCH (`v1.0.1`)

Increment for backward-compatible fixes only. Examples:

- Bug fixes in the kernel, drivers, or user programs
- Build script fixes that do not change the required environment
- Documentation corrections and typo fixes
- Small corrections to lab materials

## Pre-release Versions

Use a pre-release suffix to publish a version for testing before it is final:

| Suffix | Meaning |
|--------|---------|
| `-alpha.N` | Early, incomplete, may change significantly |
| `-beta.N` | Feature complete, still being tested |
| `-rc.N` | Release candidate, expected to become final unless problems are found |

Examples: `v1.1.0-beta.1`, `v2.0.0-rc.1`

Pre-releases sort before the final release (`v1.1.0-rc.1` < `v1.1.0`). Mark them as **pre-release** when creating the release on GitHub.

## Releasing

1. Make sure all intended changes are merged into `master`.
2. Update `CHANGELOG.md` with an entry for the new version.
3. Build the kernel from a clean checkout. The build should not report a `-dirty` version.
4. Create an annotated tag:

   ```bash
   git tag -a v1.1.0 -m "ICS-OS v1.1.0"
   git push origin v1.1.0
   ```

5. Create a GitHub Release from the tag.
   - Title: `ICS-OS v1.1.0`
   - Description: summary of changes (copy from the changelog; GitHub's "Generate release notes" can help draft it)
   - Attach the floppy image (`ics-os-floppy.img`) built from that exact tag as a release asset
   - Note the build environment used (for example, Docker image or Ubuntu version)

## Version in the Build

The version and commit hash are embedded in the kernel at build time using:

```bash
git describe --tags --always --dirty
```

Possible results:

| Output | Meaning |
|--------|---------|
| `v1.0.0` | Built exactly from the `v1.0.0` tag |
| `v1.0.0-5-g3f2a1bc` | Built 5 commits after `v1.0.0`, at commit `3f2a1bc` |
| `v1.0.0-5-g3f2a1bc-dirty` | Same, but with uncommitted local changes |
| `3f2a1bc` | No tags available; commit hash only |

Only builds that report a clean, exact tag (such as `v1.0.0`) should be distributed as official releases.

## Changelog

Every release is documented in `CHANGELOG.md`, grouped under headings such as **Added**, **Changed**, **Fixed**, and **Removed**, following the [Keep a Changelog](https://keepachangelog.com/) format.

## Tag and Branch Rules

- Release tags are never moved or deleted once published. If a release has a problem, publish a new PATCH version.
- `master` is the main development branch. Any commit on `master` after a release is considered unreleased until it is tagged.
- Older release lines (for example, `v1.x`) may be maintained on a separate branch such as `release/1.x` if fixes need to be backported.

## Course-Specific Tags (Optional)

To record the exact code used in a particular course offering, additional non-release tags may be created in the form:

```
cmsc125-YYYY-Ns
```

where `YYYY` is the academic year start and `N` is the semester (for example, `cmsc125-2026-1s`). These tags mark a point in time and do not follow the versioning rules above.
