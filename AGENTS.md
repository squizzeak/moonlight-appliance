# Agent Guide

This file applies to the entire repository. It is the concise operational guide
for coding agents and maintainers; `README.md` remains the authoritative public
installation and safety document.

## Project state

- This is the initial, manually installed iteration of Moonlight Appliance.
- The tested target is EndeavourOS/Arch with KDE Plasma 6 on Wayland.
- Automated installation, upgrades, validation, and removal are planned and
  tracked in GitHub issue #1.
- The on-screen keyboard is a complete custom PyQt6/uinput implementation. It
  is not `wvkbd` and has no upstream fetch/patch workflow.
- The project was developed and packaged through the Codex/GPT-5.6 Sol workflow
  disclosed in the README. Preserve that disclosure unless the project's
  provenance actually changes.

## Repository map

- `src/moonlight-home-button`: passive evdev controller daemon, virtual mouse,
  Moonlight/KWin integration, application switcher, and sleep monitoring.
- `src/moonlight-controller-keyboard`: custom Qt keyboard and virtual uinput
  keyboard.
- `keyboard/layer_shell_bridge.cpp`: Qt/LayerShellQt bridge that anchors the
  keyboard and reserves the bottom-third work area.
- `keyboard/CMakeLists.txt`: native bridge build.
- `systemd/user/`: deployed user-service definitions.
- `README.md`: authoritative packages, installation, behavior, safety, update,
  and repository-layout documentation.
- GitHub Wiki: approachable feature, controller, and installation guides. It is
  stored by GitHub in the separate `moonlight-appliance.wiki.git` repository.

## Non-negotiable behavior

Preserve these invariants unless the user explicitly changes the requirements:

1. Never call `grab()`, use `EVIOCGRAB`, or otherwise intercept the physical
   controller. Moonlight must receive the original controller directly.
2. Emit synthetic local input only when Moonlight is absent. Unknown
   Moonlight-class windows must fail safely by disabling synthetic input.
3. Discover controllers by capability and rescan; never bind permanently to a
   `/dev/input/eventN` path.
4. Normalize every axis using that device's evdev `AbsInfo`; never assume a
   fixed signed range or center.
5. Home/Guide acts only on a short matching release. Long holds and controller
   disconnects must not launch Moonlight.
6. Hide the keyboard when Moonlight appears, no controller exists, the last
   controller disconnects, or sleep begins.
7. Keep the keyboard non-focus-taking. Its uinput output must go to the normal
   focused application.
8. Keep the keyboard as a bottom-anchored, always-on-top layer-shell surface
   occupying and reserving one third of the primary screen's logical height.
   Geometry and labels must remain resolution- and scale-aware.
9. Preserve sticky, simultaneous keyboard modifiers and clear them when the
   keyboard hides.
10. Track held virtual buttons by physical owner so simultaneous controllers or
    overlapping bindings cannot cause stuck or premature releases.
11. Sleep-time disconnect applies only to tracked Bluetooth controllers. Do
    not deauthorize USB devices or assume a USB hub can switch VBUS power.
12. Keep a simple rollback switch for sleep-time Bluetooth disconnect behavior.

## Current controller contract

When Moonlight is absent:

- Left stick moves the pointer; right stick scrolls.
- A and right shoulder produce left click.
- Short B and left shoulder produce right click.
- Holding B for one second closes the focused window.
- D-pad produces arrow keys unless the keyboard or app switcher consumes it.
- Select toggles the keyboard.
- Holding Y opens the app switcher; D-pad navigates, Down closes the highlighted
  app, and releasing Y commits the selection.
- X is Backspace and L3 toggles sticky Shift while the keyboard is visible.
- A short Home release launches or focuses Moonlight; a two-second hold is
  ignored locally.

Do not duplicate this contract in new files. Update the existing README, Wiki,
and this guide when it changes.

## Editing and deployment discipline

- Treat repository files as the public source of truth. Do not silently edit a
  deployed `~/.local` copy without synchronizing the corresponding repository
  source, and do not deploy repository changes unless the user requests it.
- Preserve executable mode on both files under `src/`.
- Keep service paths user-relative (`%h`) and keep the daemons unprivileged.
- Never commit controller MAC addresses, transient event numbers, usernames,
  home-directory paths, tokens, logs containing private data, compiled `.so`
  files, or build directories.
- Avoid destructive USB authorization or power operations in tests.
- Do not suspend the host, close arbitrary windows, power off controllers, or
  launch GUI state-changing tests without explicit authorization.
- Preserve unrelated user changes and inspect the worktree before editing.

## Required validation

Run checks proportional to each change. Before committing a release-affecting
change, run at least:

```bash
python -c 'import ast,pathlib; [ast.parse(pathlib.Path(p).read_text()) for p in ["src/moonlight-home-button", "src/moonlight-controller-keyboard"]]'
cmake -S keyboard -B build/keyboard -DCMAKE_BUILD_TYPE=Release
cmake --build build/keyboard --parallel
systemd-analyze --user verify systemd/user/*.service
git diff --check
```

Also:

- Exercise input-state changes with mocks where possible so validation cannot
  close windows, disconnect controllers, or suspend the system.
- Test overlapping press/release ownership whenever adding a second physical
  binding for one virtual key or mouse button.
- Test disconnect cleanup for every newly introduced held state or task.
- After an authorized deployment, restart affected services and verify both
  their active state and recent journal output.
- Record hardware-dependent or visual checks that still require a person.

## Documentation must stay current

Documentation updates are part of completing a change, not optional follow-up.

- Update `README.md` whenever packages, installation, configuration, behavior,
  safety guidance, paths, build steps, or update steps change.
- Update the GitHub Wiki whenever user-facing features or controls change. If
  the Wiki checkout or GitHub access is unavailable, explicitly report the
  pending Wiki update rather than silently leaving it stale.
- Update `AGENTS.md` whenever architecture, invariants, validation requirements,
  source-of-truth rules, or agent workflow changes.
- If a maintainer has a separate machine handoff/status document, update it
  whenever deployed state, validation status, rollback information, public
  repository status, or remaining work changes. Do not publish machine-specific
  details merely to keep the public repository synchronized.
- Keep duplicated facts consistent. Prefer linking to the README over copying
  long installation sections elsewhere.

Before handing off work, explicitly check whether the README, Wiki, this file,
and any maintainer handoff are still accurate.

## Commits and public changes

- Review staged content for secrets and machine-specific data.
- Write comprehensive commit messages. Use a clear, meaningful subject and,
  for any non-trivial change, a body that records what changed, why it changed,
  important implementation or safety decisions, validation performed, and any
  known limitations or follow-up work. The commit history should be sufficient
  for a future maintainer to understand the change without reconstructing the
  entire conversation that produced it.
- Make changes through a branch and pull request unless the user explicitly
  authorizes an administrator direct commit. The public `main` branch requires
  pull requests with one approving review for ordinary contributors, while
  repository administrators may bypass that requirement and push directly.
- Administrators may also bypass the protected branch's force-push block when
  history repair or an interactive rebase genuinely requires it. Prefer
  `--force-with-lease`, verify the expected remote commit immediately before
  pushing, and never rewrite shared history casually. Keep the repository-wide
  `allow_force_pushes` setting disabled because enabling it would grant force
  push access to every collaborator with write permission, not only admins.
- Do not commit, push, open issues, or edit the Wiki unless the user requested
  that public action.
- When a public change is authorized, verify the remote state after publishing
  and include the resulting commit, issue, or Wiki URL in the handoff.
