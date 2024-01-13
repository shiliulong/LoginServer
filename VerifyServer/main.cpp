#include <QCoreApplication>
#include "VerifyManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    VerifyManager server;
    server.Initialize();
    
    return a.exec();
}
