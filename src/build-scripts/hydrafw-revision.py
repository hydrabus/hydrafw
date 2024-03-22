#!/usr/bin/env python3

from git import *
import re

r = re.compile("v(\d+\.\d+).*")

git=Repo(search_parent_directories=True).git
version = git.describe(tags=True,always=True,dirty=True,long=True)
print("VERSION:", version)
print("SEARCH:", r.search(version))
print(r.search(version).group(1))
