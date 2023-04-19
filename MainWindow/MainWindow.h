#ifndef CMAINWINDOW_H
#define CMAINWINDOW_H

#include "SABUtils/QtUtils.h"
#include "FileMaps.h"

#include <QMainWindow>
#include <QDate>
#include <QFuture>
#include <QList>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class CProgressDlg;
class CFilterModel;
class QStandardItemModel;
class CMD5Computer;
class QStandardItem;

namespace Ui { class CMainWindow; }

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget * parent = 0 );
    ~CMainWindow();

Q_SIGNALS:
public Q_SLOTS:
    void slotGo();
    void slotFinished();
    void slotSelectDir();
    void slotDirChanged();
    void slotFilesFound( const QStringList & paths );
    void slotFilesComputed( int numComputed );
    void slotFileComputing( Qt::HANDLE threadID, const QString & fileName );

    void slotAddIgnoredDir();
    void slotDelIgnoredDir();
private:
    TDirSet getIgnoredDirs() const;
    void addIgnoredDir( const QString & ignoredDir );
    void addIgnoredDirs( QStringList ignoredDirs );
    QStandardItem * addDir( const QFileInfo & dir );

    void initModel();
    CProgressDlg * fProgress{ nullptr };
    QStandardItemModel * fModel;
    std::unique_ptr< Ui::CMainWindow > fImpl;

    CMD5Computer * fMD5Computer{ nullptr };

    using TFileMap = std::unordered_map< QString, QStandardItem *, NQtUtils::CFileInfoCaseInsensitiveHash, NQtUtils::CFileInfoCaseInsensitiveEqual >;
    TFileMap fFileMap;
    int fTotalFiles{ 0 };
    int fMD5FilesComputed{ 0 };
    QDateTime fStartTime;
    QDateTime fEndTime;
};

#endif 
