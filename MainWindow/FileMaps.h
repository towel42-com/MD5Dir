#ifndef FILEMAPS_H
#define FILEMAPS_H

#include <QString>
#include "SABUtils/QtUtils.h"
#include <unordered_map>
#include <unordered_set>

using TDirSet = std::unordered_set< QString, NQtUtils::CFileInfoCaseInsensitiveHash, NQtUtils::CFileInfoCaseInsensitiveEqual >;
using TFileMap = std::unordered_map< QString, QString, NQtUtils::CFileInfoCaseInsensitiveHash, NQtUtils::CFileInfoCaseInsensitiveEqual >;
#endif 
