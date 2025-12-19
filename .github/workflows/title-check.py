cases = {
    0: {"title": "PWGJE Trigger ALICE3: Fix scope of tables in Jet.h", "labels": "pwgje,trigger"},
    1: {"title": "PWGHFF : Add centrality: (FT0C as default) bin to the ThnSparse", "labels": "pwghf"},
    2: {
        "title": "[PWGHF, Common] Add centrality (FT0C as default) bin to the ThnSparse",
        "labels": "pwgje,pwghf,trigger",
    },
    3: {"title": "PWGHF, Cmake Add centrality (FT0C as default) bin to the ThnSparse", "labels": "pwgje,pwghf,trigger"},
    4: {
        "title": "D2H,  Cmake:  Add centrality (FT0C as default) bin to the ThnSparse",
        "labels": "pwgje,pwghf,infra,trigger",
    },
    5: {"title": "PWGHF/D2H: Cmake: Add centrality (FT0C as default) bin to the ThnSparse", "labels": "pwghf"},
    6: {"title": " Add centrality (FT0C as default) bin to the ThnSparse", "labels": ""},
    7: {"title": "[PWGCF]: Add centrality (FT0C as default) bin to the ThnSparse", "labels": "pwgcf"},
    8: {"title": "[Cmake/C++] Add centrality (FT0C as default) bin to the ThnSparse", "labels": ""},
    9: {"title": " [Cmake/C--]: Add centrality (FT0C as default) bin to the ThnSparse", "labels": "common"},
    10: {"title": "[Cmake/C--]: Add centrality (FT0C as default) bin to the ThnSparse", "labels": ""},
    11: {"title": " [Cmake/C--]:  Add centrality (FT0C as default) bin to the ThnSparse ", "labels": ""},
    12: {
        "title": "PWGHF, Cmake Add centrality (FT0C as default) bin to the ThnSparse",
        "labels": "pwgje,pwghf,trigger",
    },
    13: {"title": " PWGJE Trigger  :  Fix scope of tables in Jet.h ", "labels": "pwgje,trigger"},
    14: {
        "title": " PWGHF, Cmake Add centrality (FT0C as default) bin to the ThnSparse",
        "labels": "pwgje,pwghf,trigger",
    },
    15: {"title": "[PWGJE, Trigger] :  Fix scope of tables in Jet.h ", "labels": "pwgje,trigger"},
    16: {"title": "[PWGJE, Trigger] Fix scope of tables in Jet.h ", "labels": "pwgje,trigger"},
    17: {"title": "[Cmake  C--]: Add centrality (FT0C as default) bin to the ThnSparse", "labels": ""},
}

case = 17

labels = cases[case]["labels"]
title = cases[case]["title"]

# This goes in the GitHub action.

import os
import re
import sys

# title = os.environ["title"]
# labels = os.environ["labels"]
tags = {
    "infrastructure": "Infrastructure",
    "scripts": "Scripts",
    "tools": "Tools",
    "common": "Common",
    "trigger": "Trigger",
    "dpg": "DPG",
    "tutorial": "Tutorial",
    "alice3": "ALICE3",
    "pwgcf": "PWGCF",
    "pwgdq": "PWGDQ",
    "pwgem": "PWGEM",
    "pwghf": "PWGHF",
    "pwgje": "PWGJE",
    "pwglf": "PWGLF",
    "pwgmm": "PWGMM",
    "pwgud": "PWGUD",
}
print(f'PR title: "{title}"')
print(f'PR labels: "{labels}"')
tags_relevant = [tags[label] for label in tags if label in labels.split(",")]
print("Relevant title tags:", ",".join(tags_relevant))
prefix_generated = ",".join(tags_relevant)
prefix_generated = f"[{prefix_generated}] "
print(f"Generated prefix: {prefix_generated}")
found_tags = False
title_new = title.strip()
# If there is a prefix which contains a known tag, check it for correct tags, and reformat it if needed.
# If there is a prefix which does not contain any known tag, add the tag prefix.
# If there is no prefix, add the tag prefix.
if match := re.match(r" *\[?(\w[\w,/\+\- ]+)[\]: ]+ ", title):
    prefix_title = match.group(1)
    words_prefix_title = prefix_title.replace(",", " ").replace("/", " ").split()
    title_stripped = title[len(match.group()) :].strip()
    print(f'PR title prefix: "{prefix_title}" -> tags: {words_prefix_title}')
    print(f'Stripped PR title: "{title_stripped}"')
    if any(tag in words_prefix_title for tag in tags.values()):
        found_tags = True
        passed = True
        for tag in tags.values():
            if tag in tags_relevant and tag not in words_prefix_title:
                print(f'::error::Relevant tag "{tag}" not found in the prefix of the PR title.')
                passed = False
            if tag not in tags_relevant and tag in words_prefix_title:
                print(f'::error::Irrelevant tag "{tag}" found in the prefix of the PR title.')
                passed = False
        if not passed:
            print("::error::Problems were found in the PR title prefix.")
            print('::notice::Use the form "tags: title" or "[tags] title".')
            sys.exit(1)
        # Form a valid title with the existing prefix.
        prefix_good = ",".join(w for w in prefix_title.replace(",", " ").split() if w)
        prefix_good = f"[{prefix_good}] "
        print(f'::notice::Reformatted prefix: "{prefix_good}"')
        title_new = prefix_good + title_stripped
    else:
        print("::warning::No known tags found in the prefix.")
        title_new = " ".join((*match.group().split(), title_stripped))
else:
    print("::warning::No valid prefix found in the PR title.")
if not found_tags and tags_relevant:
    title_new = prefix_generated + title_new.strip()
replace_title = 0
if title_new == title:
    print("::notice::The PR title is fine.")
else:
    replace_title = 1
    print("::warning::The PR title needs to be adjusted.")
    print(f'::warning::New title: "{title_new}".')
# with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as fh:
#     print(f"replace={replace_title}", file=fh)
#     print(f"title={title_new}", file=fh)
