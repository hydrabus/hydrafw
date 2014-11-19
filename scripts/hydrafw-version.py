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
  git=Repo().git
  print '#define HYDRAFW_GIT_TAG "' + git.describe(tags=True,always=True) + '"'
  if Repo().is_dirty:
     print '#define HYDRAFW_GIT_HASH "' + git.rev_parse('HEAD',short=True) + '-dirty"'
  else:
     print '#define HYDRAFW_GIT_HASH "' + git.rev_parse('HEAD',short=True) + '"'
  print '#define HYDRAFW_BUILD_DATE "' + date.today().isoformat() + '"'
else:
  parser.print_help()
  sys.exit(1)
