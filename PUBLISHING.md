# Publishing Top Demons

1. Create a public GitHub repository and upload the contents of this folder.
2. Open the repository's **Actions** tab and run **Build Geode Mod**.
3. Wait for all five platform jobs and the **Package builds** job to finish.
4. Download the **Build Output** artifact from the completed workflow.
5. Create a GitHub Release with tag `v1.0.0`.
6. Upload the combined `mbmb709.topdemons.geode` file to that release.
7. Copy the release asset's direct download link.
8. Sign in at <https://geode-sdk.org/login>.
9. Open <https://geode-sdk.org/me> and submit the direct download link.

For every update:

- Increase `version` in `mod.json`.
- Update `changelog.md`.
- Create a new tag and a new GitHub Release.
- Never replace an asset from an older release.
