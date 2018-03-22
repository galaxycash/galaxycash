#ifndef GALAXYCASH_CLIENTVERSION_H
#define GALAXYCASH_CLIENTVERSION_H

//
// client versioning
//

// These need to be macros, as version.cpp's and galaxycash-qt.rc's voodoo requires it
#define CLIENT_VERSION_MAJOR       2
#define CLIENT_VERSION_MINOR       3
#define CLIENT_VERSION_REVISION    0
#define CLIENT_VERSION_BUILD       0
#define CLIENT_VERSION_CODENAME    "Mars"

// Set to true for release, false for prerelease or test build
#define CLIENT_VERSION_IS_RELEASE  true

// Converts the parameter X to a string after macro replacement on X has been performed.
// Don't merge these into one macro!
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

#endif

