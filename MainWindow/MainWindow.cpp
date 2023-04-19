#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "MD5Computer.h"
#include "ProgressDlg.h"
#include "SABUtils/MD5.h"
#include "SABUtils/ButtonEnabler.h"

#include <QFileDialog>
#include <QStandardItemModel>
#include <QSettings>
#include <QProgressBar>
#include <QDirIterator>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QProgressDialog>
#include <QMessageBox>
#include <QThreadPool>
#include <QFontDatabase>
#include <random>
#include <QtConcurrent>
#include <unordered_set>

class CFilterModel : public QSortFilterProxyModel
{
public:
    CFilterModel( QObject * owner ) :
        QSortFilterProxyModel( owner )
    {
    }
    void setShowDupesOnly( bool showDupesOnly )
    {
        if( showDupesOnly != fShowDupesOnly )
        {
            fShowDupesOnly = showDupesOnly;
            invalidate();
        }
    }

    void setLoadingValues( bool loading )
    {
        if( loading != fLoadingValues )
        {
            fLoadingValues = loading;
            invalidate();
        }
    }

    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override
    {
        if( source_parent.isValid() )
            return true; // all children are visible if parent is

        if( !fShowDupesOnly )
            return true;

        QModelIndex sourceIdx = sourceModel()->index( source_row, 1, source_parent );
        auto cnt = sourceIdx.data().toString();
        if( cnt.toInt() > 1 )
            return true;
        return false;
    }

    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override
    {
        if( fLoadingValues )
            return;

        if( column == 1 )
            setSortRole( Qt::UserRole + 1 );
        else
            setSortRole( Qt::DisplayRole );
        QSortFilterProxyModel::sort( column, order );
    }

    virtual bool lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const override
    {
        return source_left.data().toInt() < source_right.data().toInt();
    }

    bool fLoadingValues{ false };
    bool fShowDupesOnly{ true };
};

CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    setWindowIcon( QIcon( ":/resources/md5dir.png" ) );
    setAttribute( Qt::WA_DeleteOnClose );

    fModel = new QStandardItemModel( this );
    initModel();
    fImpl->files->setModel( fModel );

    fImpl->go->setEnabled( false );

    connect( fImpl->go, &QToolButton::clicked, this, &CMainWindow::slotGo );
    connect( fImpl->selectDir, &QToolButton::clicked, this, &CMainWindow::slotSelectDir );
    connect( fImpl->dirName, &QLineEdit::textChanged, this, &CMainWindow::slotDirChanged );

    connect( fImpl->addDir, &QToolButton::clicked, this, &CMainWindow::slotAddIgnoredDir );
    connect( fImpl->delDir, &QToolButton::clicked, this, &CMainWindow::slotDelIgnoredDir );

    new CButtonEnabler( fImpl->ignoredDirs, fImpl->delDir );

    QSettings settings;
    fImpl->dirName->setText( settings.value( "Dir", QString() ).toString() );
    fImpl->ignoreHidden->setChecked( settings.value( "IgnoreHidden", true ).toBool() );
    addIgnoredDirs( settings.value( "IgnoredDirectories", QStringList() ).toStringList() );


    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, 100 );

    QThreadPool::globalInstance()->setExpiryTimeout( -1 );
}

void CMainWindow::addIgnoredDirs( QStringList ignoredDirs )
{
    TDirSet beenHere = getIgnoredDirs();
    for( auto ii = ignoredDirs.begin(); ii != ignoredDirs.end(); )
    {
        if( beenHere.find( *ii ) == beenHere.end() )
        {
            beenHere.insert( *ii );
            ii++;
        }
        else
            ii = ignoredDirs.erase( ii );
    }

    fImpl->ignoredDirs->addItems( ignoredDirs );
}

void CMainWindow::addIgnoredDir( const QString & ignoredDir )
{
    addIgnoredDirs( QStringList() << ignoredDir );
}

void CMainWindow::initModel()
{
    fModel->clear();
    fModel->setHorizontalHeaderLabels( QStringList() << "FileName" << "Size" << "MD5" );
}

CMainWindow::~CMainWindow()
{
    QSettings settings;
    settings.setValue( "Dir", fImpl->dirName->text() );
    settings.setValue( "IgnoreHidden", fImpl->ignoreHidden->isChecked() );
    auto ignoredDirs = getIgnoredDirs();
    QStringList tmpList;
    for( auto && ii : ignoredDirs )
        tmpList << ii;
    settings.setValue( "IgnoredDirectories", tmpList );
}

void CMainWindow::slotSelectDir()
{
    auto dir = QFileDialog::getExistingDirectory( this, "Select Directory", fImpl->dirName->text() );
    if( dir.isEmpty() )
        return;

    fImpl->dirName->setText( dir );
}

void CMainWindow::slotDirChanged()
{
    initModel();
    QFileInfo fi( fImpl->dirName->text() );
    fImpl->go->setEnabled( fi.exists() && fi.isDir() );
}

void CMainWindow::slotFileComputing( Qt::HANDLE threadID, const QString & fileName )
{
    fProgress->setCurrentMD5Info( threadID, QFileInfo( fileName ) );
    fProgress->updateActiveThreads();
}

void CMainWindow::slotFilesComputed( int numComputed )
{
    fProgress->setMD5Value( fProgress->md5Value() + numComputed );
    fProgress->updateActiveThreads();
}

TDirSet CMainWindow::getIgnoredDirs() const
{
    TDirSet ignoredDirs;
    for( int ii = 0; ii < fImpl->ignoredDirs->count(); ++ii )
    {
        ignoredDirs.insert( fImpl->ignoredDirs->item( ii )->text() );
    }
    return ignoredDirs;
}

QStandardItem * CMainWindow::addDir( const QFileInfo & dir )
{
    bool isRoot = NQtUtils::CFileInfoCaseInsensitiveEqual()( dir.absoluteFilePath(), fImpl->dirName->text() );
    auto pos = fFileMap.find( dir.absoluteFilePath() );
    QStandardItem * retVal = nullptr;
    if( pos == fFileMap.end() )
    {
        // create root node
        retVal = new QStandardItem( dir.absoluteFilePath() );
        fFileMap[ dir.absoluteFilePath() ] = retVal;
        if( isRoot )
        {
            fModel->appendRow( retVal );
        }
        else
        {
            auto parent = addDir( dir.absolutePath() );
            parent->appendRow( retVal );
        }
    }
    else
    {
        retVal = ( *pos ).second;
    }
    return retVal;
}

void CMainWindow::slotFilesFound( const QStringList & files )
{
    if( !fProgress )
        return;
    fTotalFiles += files.count();
    fProgress->setMD5Range( 0, fTotalFiles );
    for( auto && ii : files )
    {
        auto pos = fFileMap.find( ii );
        if( pos == fFileMap.end() )
        {
            QFileInfo fi( ii );
            QString dir;
            if( fi.isDir() )
                dir = fi.absoluteFilePath();
            else
                dir = fi.absolutePath();

            auto dirItem = addDir( QFileInfo( dir ) );
            if( !fi.isDir() )
            {
                auto item = new QStandardItem( ii );
                auto size = new QStandardItem( QFileInfo( fi ).size() );
                dirItem->appendRow( { item, size } );
                fFileMap[ ii ] = item;
            }
        }
    }
}

QString secsToString( qint64 seconds )
{
    const qint64 DAY = 86400;
    qint64 days = seconds / DAY;
    QTime t = QTime( 0, 0 ).addSecs( seconds % DAY );
    return QString( "%1 days, %2 hours, %3 minutes, %4 seconds" ).arg( days ).arg( t.hour() ).arg( t.minute() ).arg( t.second() );
}

void CMainWindow::slotGo()
{
    initModel();
    fImpl->files->resizeColumnToContents( 1 );
    fImpl->files->setSortingEnabled( false );
    
    fProgress = new CProgressDlg( tr( "Cancel" ), this );

    fMD5FilesComputed = 0;
    fTotalFiles = 0;

    fProgress->setMD5Format( "%v of %m - %p%" );
    fProgress->setMD5Range( 0, 0 );
    fProgress->setMD5Value( 0 );
    fProgress->show();
    fProgress->adjustSize();
    fStartTime = QDateTime::currentDateTime();
    fProgress->setRelToDir( fImpl->dirName->text() );
    qApp->processEvents();

    fMD5Computer = new CMD5Computer( fImpl->dirName->text(), getIgnoredDirs(), fImpl->ignoreHidden->isChecked() );
    connect( fMD5Computer, &CMD5Computer::sigFilesFound, this, &CMainWindow::slotFilesFound );
    connect( fMD5Computer, &CMD5Computer::sigFilesComputed, this, &CMainWindow::slotFilesComputed );
    connect( fMD5Computer, &CMD5Computer::sigFileComputing, this, &CMainWindow::slotFileComputing );// , Qt::BlockingQueuedConnection );
    connect( fProgress, &CProgressDlg::sigCanceled, fMD5Computer, &CMD5Computer::stop );
    fMD5Computer->resetStop();
    fMD5Computer->run();

    connect( fMD5Computer, &CMD5Computer::sigFinished, this, &CMainWindow::slotFinished );

    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 100, fImpl->files->columnWidth( 0 ) ) );
}

void CMainWindow::slotFinished()
{
    fEndTime = QDateTime::currentDateTime();

    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 100, fImpl->files->columnWidth( 0 ) ) );
    fProgress->setMD5Finished();

    fProgress->hide();
    fProgress->deleteLater();

    fImpl->files->setSortingEnabled( true );

    QLocale locale;
    QMessageBox::information( this, "Finished",
                              tr(
                              "<ul><li>Results: Number of Files: %2 processed</li>"
                              "<li>Elapsed Time: %4</li>"
    ).arg( locale.toString( fTotalFiles ) ).arg( secsToString( fStartTime.secsTo( fEndTime ) ) ) );
}

void CMainWindow::slotAddIgnoredDir()
{
    auto dir = QFileDialog::getExistingDirectory( this, "Select Directory", fImpl->dirName->text() );
    if( dir.isEmpty() )
        return;

    addIgnoredDir( dir );
}

void CMainWindow::slotDelIgnoredDir()
{
    auto curr = fImpl->ignoredDirs->currentItem();
    if( !curr )
        return;

    delete curr;
}
