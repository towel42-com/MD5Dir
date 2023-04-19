#include "ProgressDlg.h"
#include "ui_ProgressDlg.h"

#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QLocale>
#include <QThreadPool>
#include <QCloseEvent>

CProgressDlg::CProgressDlg( QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CProgressDlg )
{
    fImpl->setupUi( this );
    setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );
    setWindowFlags( windowFlags() & ~Qt::WindowCloseButtonHint );

    fImpl->md5Text->setTextFormat( Qt::TextFormat::RichText );
    fImpl->md5Text->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    connect( fImpl->cancelButton, &QPushButton::clicked, this, &CProgressDlg::slotCanceled );
    setStatusLabel();
}

CProgressDlg::CProgressDlg( const QString & cancelText, QWidget * parent ) :
    CProgressDlg( parent )
{
    fImpl->md5Text->setText( tr( "Computing MD5s..." ) );
    setCancelText( cancelText );
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::closeEvent( QCloseEvent * event )
{
    slotCanceled();
    event->accept();
}

void CProgressDlg::slotCanceled()
{
    fCanceled = true;
    emit sigCanceled();
}


void CProgressDlg::setMD5Value( int value )
{
    fImpl->md5Progress->setValue( value );
}

int CProgressDlg::md5Value() const
{
    return fImpl->md5Progress->value();
}

void CProgressDlg::setMD5Range( int min, int max )
{
    fImpl->md5Progress->setRange( min, max );
}

int CProgressDlg::md5Min() const
{
    return fImpl->md5Progress->minimum();
}

int CProgressDlg::md5Max() const
{
    return fImpl->md5Progress->maximum();
}

void CProgressDlg::setMD5Format( const QString & format )
{
    fImpl->md5Progress->setFormat( format );
}

QString CProgressDlg::md5Format() const
{
    return fImpl->md5Progress->format();
}


void CProgressDlg::setCurrentMD5Info( Qt::HANDLE /*threadID*/, const QFileInfo & /*fileInfo*/ )
{
    //QLocale locale;

    //QStringList threadList;
    //for( auto && ii : fThreadMap )
    //{
    //    auto threadID = tr( "%1" ).arg( reinterpret_cast< uintptr_t >( ii.first ), 6, 16, QChar( '0' ) ).right( 6 );
    //    threadList << tr( "<li>ThreadID: 0x%1 - File '%2' (%3 bytes)</li>" ).arg( threadID ).arg( fRelToDir.relativeFilePath( ii.second.absoluteFilePath() ) ).arg( locale.toString( ii.second.size() ) );
    //}
    //fImpl->md5Text->setText( tr( "<ul>%1</ul>" ).arg( threadList.join( "\n" ) ) );
}

void CProgressDlg::updateActiveThreads()
{
    auto threadPool = QThreadPool::globalInstance();
    auto numActive = threadPool->activeThreadCount();
    fImpl->numThreads->setText( tr( "Num Active Threads: %1" ).arg( numActive ) );
}

void CProgressDlg::setCancelText( const QString & label )
{
    fImpl->cancelButton->setText( label );
}

QString CProgressDlg::cancelText() const
{
    return fImpl->cancelButton->text();
}

void CProgressDlg::setStatusLabel()
{
    //QStringList text;
    //if( !fMD5Finished )
    //    text << tr( "Computing MD5s" );

    //if( text.isEmpty() )
    //    fImpl->mainLabel->setText( tr( "Finished" ) );
    //else
    //    fImpl->mainLabel->setText( text.join( " and " ) + "..." );
}

void CProgressDlg::setRelToDir( const QDir & relToDir )
{
    fRelToDir = relToDir;
}

void CProgressDlg::setMD5Finished()
{
    fMD5Finished = true;

    setStatusLabel();
}
