#include "MainWindow/MainWindow.h"

#include <QApplication>
#include <QMessageBox>

int main( int argc, char ** argv )
{
    QApplication appl( argc, argv );
    Q_INIT_RESOURCE( application );

    QCoreApplication::setOrganizationName( "Towel42 Consulting Services" );
    QCoreApplication::setApplicationName( "MD5 Dir" );
    QCoreApplication::setApplicationVersion( "1.0.0" );
    QCoreApplication::setOrganizationDomain( "www.towel42.com" );
    appl.setWindowIcon( QPixmap( ":/resources/md5dir.png" ) );

    CMainWindow * wnd = new CMainWindow;
    wnd->show();
    return appl.exec();
}

