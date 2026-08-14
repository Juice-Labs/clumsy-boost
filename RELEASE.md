# Releasing clumsy-boost

Produces a versioned GitHub Release with prebuilt x64 + x32 zips attached. See
`BUILD.md` for the build details this depends on.

## Order of operations (important)

Tag and release from **merged `master`**, not from a feature branch. Tagging a
branch tip before merge means the tag points at a commit that isn't the released
state (GitHub creates a merge/squash commit), which breaks "which build is vX.Y.Z".

1. **Merge the PR** to `master` (needs an approval from someone other than the author).
2. Sync local master:
   ```
   git checkout master
   git pull
   ```
3. **Bump the version** if not already done: set `CLUMSY_VERSION` in
   `src/common.h` to match the tag (it drives the window title, e.g. `"1.1.0"`).
   Commit it to master.
4. **Build both architectures** (see BUILD.md) — Release x64 and Win32.
5. **Package the zips** (PowerShell). Bump the version in the filenames:
   ```powershell
   $base = "C:\Users\polin\Projects\clumsy-boost"
   Compress-Archive -Path "$base\bin\vs\Release\x64\clumsy.exe","$base\bin\vs\Release\x64\WinDivert.dll","$base\bin\vs\Release\x64\WinDivert64.sys","$base\bin\vs\Release\x64\iup.dll","$base\bin\vs\Release\x64\config.txt" -DestinationPath "$base\clumsy-boost.v1.1.0.x64.zip" -Force
   Compress-Archive -Path "$base\bin\vs\Release\x32\clumsy.exe","$base\bin\vs\Release\x32\WinDivert.dll","$base\bin\vs\Release\x32\WinDivert32.sys","$base\bin\vs\Release\x32\WinDivert64.sys","$base\bin\vs\Release\x32\iup.dll","$base\bin\vs\Release\x32\config.txt" -DestinationPath "$base\clumsy-boost.v1.1.0.x32.zip" -Force
   ```
   (The x32 package ships both `.sys` files by design: WinDivert loads the 64-bit
   driver when a 32-bit exe runs on 64-bit Windows.)
6. **Tag the merged commit** and push the tag:
   ```
   git tag -a v1.1.0 -m "clumsy-boost v1.1.0"
   git push origin v1.1.0
   ```
7. **Create the GitHub Release** and upload the two zips as assets:
   - <https://github.com/Juice-Labs/clumsy-boost/releases/new>
   - Choose the tag, add a title + notes, drag in both `.zip` files, Publish.

## Notes

- **`gh` CLI can't create the release/PR here** — the machine's `gh` is
  authenticated as an HP enterprise-managed account, which GitHub blocks from
  Juice-Labs content. Use the web UI. (`git push` works because it uses the
  personal account's HTTPS credential.)
- The zips are **build artifacts** — they're gitignored and attach to the Release
  only; do not commit them.
- Keep `CLUMSY_VERSION`, the git tag, and the zip filenames in sync.
- Rebuild + re-package if any change lands during PR review, so the released
  binaries match the tagged commit.
