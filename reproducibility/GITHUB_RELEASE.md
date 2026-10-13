# Publishing InFuse

Use this folder as the repository root. Include README.md, INSTALL_WINDOWS.txt, source, scripts, tests, examples, reproducibility records, licenses, `.gitignore`, `.gitattributes`, and `.github`. Do not upload the parent development workspace or local build SDKs.

Before public release, resolve the publication permissions in LICENSE_STATUS.md, complete submission.txt, choose the actual paper figure/table and data, and add the author-signed review form. Add InFuse's own paper citation; REFERENCES.bib currently contains the upstream method's reference.

Create the GitHub repository named `InFuse` in your account. The author chooses the repository visibility and account; no repository URL is assumed in this package. Commit the source, inspect `git status`, and push to that repository. Preserve the original upstream notices even though the application is named InFuse.

Run the Windows workflow included in `.github/workflows/windows.yml` and confirm its result. It compiles, tests, and generates the selected bunny result data. Its hosted Windows runner is an additional check, not a clean Windows 11 installation test.

For a GRSI submission, reference a fixed commit or release rather than a moving branch. Attach the source archive and the representative result data to the release. Record the release/commit in submission.txt and the signed form. Keep measured reference data with the exact experiment manifest. An immutable scholarly archive/DOI can be added if required by the venue; no archival record has been created by this preparation.

The selected preparation repository is https://github.com/haoxu99/InFuse and is private. Its owner is haoxu99. This does not establish a public release or GRSI submission; publication metadata, release permissions, and the signed review form remain separate author actions.
