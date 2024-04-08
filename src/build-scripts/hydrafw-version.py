#!/usr/bin/env python3

import sys
import os
import re
from optparse import OptionParser
from datetime import *
from git import *

if __name__=="__main__":
  usage = """
%prog outfile.txt"""

parser = OptionParser(usage=usage)
(options, args) = parser.parse_args()
if len(args)==1:
  sys.stdout = open(args[0], 'w')
  git=Repo(search_parent_directories=True).git
  vermagic=git.describe(tags=True,always=True,dirty=True,long=True)
  if os.environ.get("GITHUB_CI_PR_SHA", "") != "":
    # if GITHUB_CI_PR_SHA is set, then it's GitHub Actions build, so workaround to set a valid tag is required
    sha_id=os.environ["GITHUB_CI_PR_SHA"][:7]
    print('#define HYDRAFW_GIT_TAG "' + re.sub('-g.*$', '-g' + sha_id, vermagic) + '"')
  else:
    # If it's local build, then set "normal" tag
    print('#define HYDRAFW_GIT_TAG "' + vermagic + '"')
  print('#define HYDRAFW_CHECKIN_DATE "' + git.show(['-s', '--pretty=format:%ai']).partition(' ')[0] + '"')
else:
  parser.print_help()
  sys.exit(1)
