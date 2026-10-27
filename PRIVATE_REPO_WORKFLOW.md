i# ICS-OS Development Workflow: Private Repo with a Public Upstream

This document describes how to maintain a **private development repository**
for ICS-OS alongside the **public repository**
(`https://github.com/srg-ics-uplb/ics-os`). All fixes, experiments, and lab
answer keys are developed in the private repo; selected changes are published
back to the public repo deliberately.

Why this setup: the private repo holds material that must never become
visible to students (lab solution patches, draft exams) mixed with fixes that
eventually should be public. GitHub does not allow a private fork of a public
repository, so we maintain an independent private mirror with two remotes.

## 1. One-time setup

### 1.0 Authenticate over HTTPS

GitHub no longer accepts account passwords for Git over HTTPS. Use one of:

**Option A: GitHub CLI (simplest).**

```bash
gh auth login            # choose: GitHub.com -> HTTPS -> authenticate via browser
gh auth setup-git        # lets git use gh as its credential helper
```

**Option B: Personal access token (PAT) with a credential helper.**
Create a token at GitHub -> Settings -> Developer settings -> Personal access
tokens. A fine-grained token needs *Contents: Read and write* on the private
repo; a classic token needs the `repo` scope. Then let git remember it:

```bash
git config --global credential.helper store     # plain text in ~/.git-credentials
# or, more securely: macOS "osxkeychain", Windows "manager",
# Linux "libsecret" / "cache --timeout=3600"
```

The first push or clone prompts for a username (your GitHub username) and a
password (paste the **token**, not your account password); the helper saves it
for later.

If the organization uses SAML SSO, authorize the token for the organization
(Token settings -> Configure SSO), otherwise pushes fail with a 403.

Never put a token inside a remote URL (`https://TOKEN@github.com/...`): it
ends up in `.git/config` and in shell history.

### 1.1 Create the private mirror

```bash
# Create an EMPTY private repo on GitHub first (no README), e.g.
#   srg-ics-uplb/ics-os-private

# Mirror the public repository into it
git clone --bare https://github.com/srg-ics-uplb/ics-os.git
cd ics-os.git
git push --mirror https://github.com/srg-ics-uplb/ics-os-private.git
cd .. && rm -rf ics-os.git
```

### 1.2 Working clone with two remotes

```bash
git clone https://github.com/srg-ics-uplb/ics-os-private.git ics-os
cd ics-os
git remote add upstream https://github.com/srg-ics-uplb/ics-os.git

# SAFETY: make pushing to the public repo impossible by accident.
git remote set-url --push upstream DISABLE
```

After this, `origin` is the private repo (fetch + push) and `upstream` is the
public repo (fetch only). `git push upstream` now fails loudly; publishing to
the public repo is always an explicit, deliberate act (section 5).

Every collaborator with private access repeats 1.2 on their machine.

## 2. Branch layout (private repo)

| Branch | Purpose | Rules |
|---|---|---|
| `master` | Mirror of public `upstream/master` | Never commit here directly; updated only by fast-forward from upstream (section 3) |
| `dev` | Integration branch for all private work (what you build and run day to day) | Topic branches merge in here; never published wholesale, never used as a branch point |
| `fix/*`, `feature/*` | One topic branch per change (`fix/thread-join`, `feature/lottery-plugin`) | **Branched from `master`**; **one concern per branch**; must contain no answer-key material |
| `solutions/*` | Lab answer keys (`solutions/lottery-lab`, `solutions/memlab`) | Stay private forever; never merged toward anything public |

The single most important discipline: **never mix solution files into a fix
branch.** A fix branch that stayed clean can be published with one merge; a
contaminated one requires commit archaeology. Mechanical enforcement options:
keep all answer keys under a `solutions/` directory and add a CI check in the
private repo that rejects `solutions/**` paths on any branch not named
`solutions/*`.

## 3. Routine: syncing public changes IN

Run whenever the public repo moves (merged student PRs, public fixes):

```bash
git checkout master
git fetch upstream
git merge --ff-only upstream/master   # ff-only: master stays an exact mirror
git push origin master

git checkout dev
git merge master                      # bring public changes into private work
```

Notes:

- `--ff-only` will refuse to proceed if someone accidentally committed to
  `master` — that is the point. Fix the stray commit (move it to a topic
  branch) rather than forcing.
- Use `merge`, not `rebase`, on `dev` if the branch is shared with
  collaborators; rebasing shared history breaks everyone else's clones. A
  solo maintainer may use `git rebase master` for linear history.
- Sync **frequently**. Conflicts surface in small, manageable doses instead
  of one giant reconciliation at semester's end.

## 4. Routine: fixing a bug or adding a feature

This is the day-to-day loop. Every change, whether a one-line bug fix or a new
kernel feature, follows the same six steps.

```
upstream/master --> master --> fix/<topic> --> (test) --> dev --> publish --> sync back
                      ^                                                         |
                      +---------------------------------------------------------+
```

### 4.1 Start from an up-to-date `master`

```bash
git checkout master
git fetch upstream
git merge --ff-only upstream/master
git checkout -b fix/<topic>            # or feature/<topic>
```

Branch from `master`, **not** from `dev`. `dev` accumulates every unpublished
change; a branch cut from it drags all of those along, so merging it toward the
public repo would publish work you never meant to. Cut from `master`, the
branch holds exactly your change and nothing else.

Name branches `fix/<short-topic>` for bugs and `feature/<short-topic>` for new
functionality (`fix/childwait-leak`, `feature/sched-command`).

### 4.2 Make the change

- Keep to **one concern per branch**. If you notice an unrelated bug while
  working, note it and fix it on its own branch.
- Commit in small steps. Write the subject line in the imperative ("Fix
  childwait leak in kill_thread") and use the body to say *why*, since the
  diff already shows *what*.
- ICS-OS builds as one kernel image, and a file can be compiled without being
  wired in. When you add a source file, also update everything that lets the
  kernel find it: the `Makefile` object list and rule, `lscript.ld`, and any
  `#include` or install call. A missing link input only shows up when you link.
- Keep answer keys, solution patches, and draft exams out of this branch.
  Before you push, check what the branch contains:

```bash
git log --stat master..HEAD            # commits and files touched
git diff --stat master...HEAD          # net change against the public baseline
git diff --name-only master...HEAD | grep -i solution   # must print nothing
```

### 4.3 Test before it merges anywhere

Build from clean, then boot and exercise the change. A kernel bug usually
shows up as a hang at boot or in the first few commands, so a build that
compiles is not yet a passing test.

```bash
make clean && make                     # inside the Docker build container
make floppy                            # build the boot image
qemu-system-i386 -fda ics-os-floppy.img
```

Test checklist:

1. The kernel builds from clean with no new errors.
2. It boots to the shell and basic commands (`ps`, `mem`, `dir`) still work.
3. The specific fix or feature does what it should. For a fix, **reproduce the
   bug first on the unpatched build**, then confirm the patch removes it.
4. Related paths still work (a scheduler change: run two CPU-bound programs
   for a few minutes; a thread change: run the threads demo several times in
   a row, since some bugs only appear on the second run).
5. For an SDK or `contrib/` change, rebuild the affected apps, since they link
   the SDK sources directly.

Record what you tested in the PR description or the last commit message.

### 4.4 Integrate into `dev`

Push the topic branch to the private repo and merge it into `dev`, ideally
through a pull request so a second person reviews it:

```bash
git push origin fix/<topic>
# open a PR: fix/<topic> -> dev on the private repo; review; merge

# or, working solo:
git checkout dev
git merge --no-ff fix/<topic>          # --no-ff keeps the topic visible in history
git push origin dev
```

`dev` is where you run and demo everything together. It is never used as the
source for publishing (section 5).

### 4.5 Changes that depend on each other

If fix B needs fix A and A is not yet published, branch B from A instead of
from `master`:

```bash
git checkout fix/a
git checkout -b fix/b
```

Publish A first, then B. After A is merged upstream and synced into `master`,
rebase B onto `master` (`git rebase master`) so its diff shrinks to only B's
own changes. Mention the dependency in B's pull request.

### 4.6 A feature that also needs a lab or answer key

Split it by audience:

- **Infrastructure** that a lab needs but that does not give the lab away (a
  kernel bug fix, a new console command, an SDK function) goes on a normal
  `fix/*` or `feature/*` branch and gets published.
- **The solution** (the code students are asked to write) goes on a
  `solutions/<lab>` branch, cut from the infrastructure branch or from
  `master` after that branch is published. It never merges toward public.

The thread exit/join work is an example of the first kind: students need it to
exist, so it is published. The lottery scheduler implementation is the second
kind: it stays on `solutions/lottery-lab`.

### 4.7 Urgent fix during a running lab

When students are blocked by a bug in the public repo, shorten the loop:
branch from `master`, fix and test, merge into `dev` for your own use, and
publish right away (section 5) instead of waiting for a batch. Students then
get the fix by pulling the public repo as usual.

### 4.8 Clean up

Once the change is published, merged upstream, and synced back into `master`
(section 3), delete the topic branch so stale branches do not pile up:

```bash
git branch --merged master             # confirm the branch is listed here
git branch -d fix/<topic>
git push origin --delete fix/<topic>
```

If `git branch -d` refuses, either the branch has commits that never reached
`master`, or the public repo merged the pull request with "squash and merge",
which rewrites the commits so git cannot tell they are the same change. To
tell the two cases apart, compare content instead of history:

```bash
git diff master fix/<topic> -- <the files you changed>   # empty = already in master
```

If that diff is empty, the change is in `master` and `git branch -D` is safe.

## 5. Routine: publishing selected fixes OUT

Never merge `dev` into the public repo — it contains private material.
Publish per topic. Because topic branches start from `master` (section 4),
a finished topic branch is already publishable as it is; the `publish/*`
branch below is only needed to combine several topics or to cherry-pick:

```bash
git checkout master                       # public baseline
git checkout -b publish/thread-fixes

# Either merge a clean topic branch...
git merge fix/thread-join
# ...or cherry-pick specific commits:
# git cherry-pick <sha> <sha>

# Push to the PUBLIC repo by explicit URL (upstream pushes are disabled):
git push https://github.com/srg-ics-uplb/ics-os.git publish/thread-fixes:thread-fixes
```

This push needs write access to the public repo. If you only have read access
there, push the branch to your own fork of the public repo instead and open the
pull request from that fork. The credential helper from section 1.0 supplies
the token for both repos, but a fine-grained token is scoped to specific
repos, so add the public repo (or fork) to the token's repository list.

Then open a pull request on the public repo from the `thread-fixes` branch
and review/merge it there like any contribution. After it merges, the next
sync (section 3) brings it back around into `master` and `dev`, closing the
loop.

Publish promptly once a fix is stable. The permanent private delta should be
limited to what genuinely must stay private (answer keys) — not an
ever-growing pile of unpublished improvements, which makes every future
publication harder.

## 6. Lab answer keys as regenerable patches

Answer keys are distributed to instructors as `.patch` files generated from
`solutions/*` branches, so they automatically track upstream drift:

```bash
# regenerate the key for the lottery lab against current public master
git checkout solutions/lottery-lab
git rebase master                       # solutions branches are private:
                                        # rebasing them is safe and keeps
                                        # the diff minimal
git diff master solutions/lottery-lab > lottery-plugin-solution.patch
```

At the start of each semester (and after any significant public merge),
re-verify every key still applies:

```bash
git checkout master
git apply --check lottery-plugin-solution.patch && echo OK
```

A failed check means the lab handout may also need updating — treat the
patch check as the canary.

## 7. Quick reference

```bash
# sync public -> private
git checkout master && git fetch upstream && git merge --ff-only upstream/master
git push origin master && git checkout dev && git merge master

# start a fix or feature (always from an up-to-date master)
git checkout master && git fetch upstream && git merge --ff-only upstream/master
git checkout -b fix/<topic>          # or feature/<topic>

# check that the branch contains only what you intend
git log --stat master..HEAD
git diff --stat master...HEAD

# land it privately (after build + boot test)
git push origin fix/<topic>          # open a PR into dev, or merge locally:
git checkout dev && git merge --no-ff fix/<topic> && git push origin dev

# clean up after it has been published and synced back
git branch -d fix/<topic> && git push origin --delete fix/<topic>

# publish a fix publicly
git checkout master && git checkout -b publish/<topic> && git merge fix/<topic>
git push https://github.com/srg-ics-uplb/ics-os.git publish/<topic>:<topic>

# regenerate an answer key
git diff master solutions/<lab> > <lab>-solution.patch
```

## 8. Rules summary

1. `master` is a mirror: fast-forward only, never commit to it.
2. `git push upstream` is disabled; publishing is always by explicit URL.
3. Topic branches start from `master`, never from `dev`.
4. One concern per topic branch; no answer-key files outside `solutions/*`.
5. `dev` is never merged or pushed to the public repo.
6. Build and boot-test every change before it merges into `dev`.
7. Sync from upstream often; publish stable fixes promptly.
8. Delete a topic branch once its change is back in `master` through a sync.
9. Re-check all solution patches against `master` after every sync.
