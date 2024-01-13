#include <QCoreApplication>
#include "LoginManager.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    LoginManager loginManager;
    loginManager.Initialize();
    
    return a.exec();
}
