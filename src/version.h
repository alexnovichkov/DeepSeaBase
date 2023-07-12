#ifndef VERSION_H
#define VERSION_H

#include <string>
#include <vector>

int CompareVersions(const std::string& verA, const std::string& verB);

// Split version string into individual components. A component is continuous
// run of characters with the same classification. For example, "1.20rc3" would
// be split into ["1",".","20","rc","3"].
std::vector<std::string> SplitVersionString(const std::string& version);

#endif // VERSION_H
