find . -name \*.m4|xargs dos2unix -f
find . -name \*.ac|xargs dos2unix -f 
find . -name \*.am|xargs dos2unix -f 
find . -name \*.pc|xargs dos2unix -f 
find . -name \*.in|xargs dos2unix -f 
find . -name \*.status|xargs dos2unix -f 
find . -name \*.guess|xargs dos2unix -f 
find . -name \*.sub|xargs dos2unix -f 
find . -name \*.sh|xargs dos2unix -f 
find . -name Makefile|xargs dos2unix -f 
find . -name libtool|xargs dos2unix -f 
find . -name install-sh|xargs dos2unix -f 
find . -name missing|xargs dos2unix -f
find . -name \*.c|xargs dos2unix -f
find . -name \*.cpp|xargs dos2unix -f
find . -name \*.h|xargs dos2unix -f
find . -name \*.clang-format|xargs dos2unix -f
