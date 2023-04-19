#include "MD5Computer.h"

#include "SABUtils/MD5.h"


#include <QThreadPool>
#include <QtConcurrent>
#include <QtGlobal>
#include <QApplication>
#include <QThreadPool>

CMD5Computer::CMD5Computer( const QString & path, const TDirSet & ignoredDirs, bool ignoreHidden ) :
    fIgnoredDirs( ignoredDirs ),
    fIgnoreHidden( ignoreHidden )
{
    fPath = QFileInfo( path ).absoluteFilePath();
    connect( &fWatcher, &QFutureWatcher< QString >::finished, this, &CMD5Computer::slotFinished );
}

void CMD5Computer::slotFinished()
{
    fMD5 = fCompletedPaths[ fPath ].first;
    emit sigFinished( fPath );
}

QString CMD5Computer::getMD5() const
{
    Q_ASSERT( fMD5.has_value() );
    if( fMD5.has_value() )
        return fMD5.value();
    return QString();
}

void CMD5Computer::run()
{
    QFileInfo fi( fPath );
    Q_ASSERT( fi.isDir() );
    if( fi.isDir() )
    {
        auto future = QtConcurrent::run( QThreadPool::globalInstance(),
                                     [this]()
        {
            return computeMD5s( fPath, "." );
        } );
        fWatcher.setFuture( future );
    }
}

bool CMD5Computer::sStopped{ false };
void CMD5Computer::resetStop()
{
    sStopped = false;
}

void CMD5Computer::stop()
{
    sStopped = true;
}

QString CMD5Computer::computeMD5s( const QString & parentDir, QString dirName )
{
    QDir dir( parentDir );
    if( !dir.exists() )
        return QString();
    if( QFileInfo( dirName ).isRelative() )
    {
        dir.cd( dirName );
    }
    else
        dir = QDir( dirName );
    if( !dir.exists() )
        return QString();

    dirName = dir.relativeFilePath( dirName );
    //qDebug() << "Computing MD5 for dir: " << parentDir << " - " << dirName;

    QDirIterator di( dir.absolutePath(), QStringList() << "*", QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags );

    QStringList paths;
    int filesFound = 0;
    while( !sStopped && di.hasNext() )
    {
        auto curr = dir.absoluteFilePath( di.next() );
        QFileInfo fi( curr );
        if( fi.isSymbolicLink() || fi.isSymLink() )
            continue;
        if( fIgnoreHidden && ( fi.isHidden() || fi.fileName().startsWith( "." ) ) )
            continue;

        if( fi.isDir() && ( fIgnoredDirs.find( curr ) != fIgnoredDirs.end() ) )
            continue;
        if( !fi.isDir() )
            filesFound++;
        paths << curr;
    }

    emit sigFilesFound( paths );


    QFutureSynchronizer< std::pair< QString, QString > > dirSync;
    for( int ii = 0; !sStopped && ( ii < paths.count() ); ++ii )
    {
        auto && curr = paths[ ii ];
        QFileInfo fi( curr );

        if( sStopped )
            break;

        if( fi.isDir() )
        {
            auto md5 = computeMD5s( curr, "." );
            QMutexLocker locker( &fMutex );
            fCompletedPaths[ curr ].first = md5;
        }
        else
        {
            dirSync.addFuture( QtConcurrent::run( QThreadPool::globalInstance(),
                                             [this, curr]()
            {
                updateThreadMap( QThread::currentThreadId(), curr );
                emit sigFileComputing( QThread::currentThreadId(), curr );
                auto md5 = NUtils::getMd5( curr, true );
                return std::make_pair( curr, md5 );
            } ) );
        }
    }


    //qApp->processEvents();

    dirSync.waitForFinished();

    auto futures = dirSync.futures();
    emit sigFilesComputed( futures.count() );
    QString key;
    for( auto && ii : futures )
    {
        if( sStopped )
            break;
        //updateThreadMap( )
        key += ii.result().second;
    }
    if( sStopped )
        return QString();

    //emit sigFilesComputed( dir.absolutePath(), numFilesInDir );

    auto md5 = NUtils::getMd5( key, false );
    return md5;
}

void CMD5Computer::updateThreadMap( Qt::HANDLE threadID )
{
    QMutexLocker locker( &fMutex );
    auto pos = fThreadMap.find( threadID );
    if( pos != fThreadMap.end() )
        fThreadMap.erase( pos );
}

void CMD5Computer::updateThreadMap( Qt::HANDLE threadID, const QFileInfo & fileInfo )
{
    QMutexLocker locker( &fMutex );
    auto pos = fThreadMap.find( threadID );
    if( pos == fThreadMap.end() )
        int xyz = 0;
    fThreadMap[ threadID ] = fileInfo;
}
