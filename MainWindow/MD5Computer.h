#ifndef MD5COMPTUER_H
#define MD5COMPTUER_H
#include "SABUtils/QtUtils.h"
#include "FileMaps.h"

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QFutureSynchronizer>
#include <map>
#include <optional>
class QThreadPool;
class CMD5Computer : public QObject
{
    Q_OBJECT;
public:
    CMD5Computer( const QString & path, const TDirSet & ignoredDirs, bool ignoreHidden );
    virtual ~CMD5Computer() {}
    void run();
    void resetStop();
    void stop();
    bool isFinished() const { return fMD5.has_value(); }
    QString getMD5() const; // run must be called first, this is a blocking call, if you cant block connect to sigFinished
Q_SIGNALS:
    void sigFinished( const QString & path );
    void sigFilesFound( const QStringList & paths );
    void sigFilesComputed( int numComputed );
    void sigFileComputing( Qt::HANDLE threadID, const QString & path );
private Q_SLOTS:
    void slotFinished();
private:
    void updateThreadMap( Qt::HANDLE threadID, const QFileInfo & fi );
    void updateThreadMap( Qt::HANDLE threadID );
    QString computeMD5s( const QString & parentDir, QString dirName );

    QString fPath;
    std::optional< QString > fMD5;
    TDirSet fIgnoredDirs;
    bool fIgnoreHidden{ true };
    static bool sStopped;

    QMutex fMutex;
    QFutureWatcher< QString > fWatcher;
    using TFileToMD5 = std::map< QString, QString, NQtUtils::CFileInfoCaseInsensitiveLessThan >;
    std::map< QString, std::pair< QString, TFileToMD5 >, NQtUtils::CFileInfoCaseInsensitiveLessThan > fCompletedPaths;

    std::map< Qt::HANDLE, QFileInfo > fThreadMap;
};
#endif
