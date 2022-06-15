# PWGHF: Heavy-flavour analysis framework

Documentation of the HF analysis framework is available at

<https://aliceo2group.github.io/analysis-framework/docs/framework/pwghf.html>

## Code development guidelines

- Follow the [O2 coding guidelines](https://github.com/AliceO2Group/CodingGuidelines)
    (especially the [naming](https://rawgit.com/AliceO2Group/CodingGuidelines/master/naming_formatting.html)
    and [commenting](https://rawgit.com/AliceO2Group/CodingGuidelines/master/comments_guidelines.html) rules).
    (It will save you time with renaming and formatting.)
- If your changes consist of several independent steps, keep them separate in several commits.
- Give your commits meaningful titles.
  - If needed, add more details in the commit message (separated by a blank line from the commit title).
- Keep your feature branch up to date with the upstream main branch.
- Test your code before making a pull request.
  - Propagate your changes into the Run3Analysisvalidation configuration.
  - Check that your branch compiles.
  - Check that your code works and runs without errors and warnings.
    - Make sure your code is compatible with the expected input (Run 2/3/5, real/MC data, p–p/Pb–Pb).
    - Check that your changes do not alter unexpectedly the control plots produced by the validation framework.
  - Make sure your tasks can be fully configured from Run3Analysisvalidation and AliHyperloop.

### Naming conventions

Use the `<object><attribute>` (or `<general><specific>`) naming scheme, so that names of the same kind of objects start with same string and the different attributes follow.
This scheme makes names more readable, searchable and sortable.

Example:

    ptTrackMin, etaTrackMax, trackPos, trackNeg

is more readable and sortable than

    minTrackPt, maxTrackEta, posTrack, negTrack

- Names of task configurables follow the same conventions as
    [names of variables](https://rawgit.com/AliceO2Group/CodingGuidelines/master/naming_formatting.html#Variable_Names).
- Names of histograms start with `h` and follow the same conventions as names of variables.
  - Names of histograms of MC variables have the following suffixes:
    - `Gen` - generator level quantity of a signal particle
    - `GenSig` - generator level quantity of a reconstructed signal candidate
    - `RecSig` - reconstruction level quantity of a reconstructed signal candidate
    - `RecBg` - reconstruction level quantity of a reconstructed background candidate

The device name of a task is automatically generated from the name of the corresponding `struct` by replacing uppercase letters with lowercase letters preceded with a hyphen unless defined explicitly using `TaskName`, which should be avoided if not necessary.

## Pull requests (PR)

- Create one PR per feature (i.e. do not mix big unrelated code changes).
- Give your PR a short meaningful title.
  - Add the “PWGHF: ” prefix in the title of your PR.
        (It allows to search for PWGHF-related PRs in the commit history of the main branch.)
    - Note: If your PR has only one commit, add the prefix also in the commit title
            (because that is the title that will appear in the history after merging).
- Give further useful details about your changes in the PR description.
  - Add links to all related PRs (e.g. O2Physics, O2, AliPhysics, Run3Analysisvalidation) in the PR description.

### PR review

- When you implement changes during the review, push them into your branch as new separate commits (with meaningful titles).
- Do not amend, squash or rebase existing commits in the PR. It would break the links between the code and the review comments.
  - If you need to update your branch with the changes in the main branch, use `merge` instead of `rebase` to preserve the commit history.
- Fix formatting issues by merging the PRs created automatically by the CI tests in your fork repository.
