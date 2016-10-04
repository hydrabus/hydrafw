import sys
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
  print('#define HYDRAFW_GIT_TAG "' + git.describe(tags=True,always=True,dirty=True,long=True) + '"')
  print('#define HYDRAFW_CHECKIN_DATE "' + git.show(['-s', '--pretty=format:%ai']).partition(' ')[0] + '"')
else:
  parser.print_help()
  sys.exit(1)
