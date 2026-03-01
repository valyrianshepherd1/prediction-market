#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVBoxLayout>

class MarketWindow : public QMainWindow {
    Q_OBJECT

public:
    MarketWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Prediction Market");
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout;

        QLabel *marketLabel = new QLabel("Markets will be listed here.", this);
        layout->addWidget(marketLabel);

        QPushButton *fetchMarketsButton = new QPushButton("Fetch Markets", this);
        connect(fetchMarketsButton, &QPushButton::clicked, this, &MarketWindow::fetchMarkets);
        layout->addWidget(fetchMarketsButton);

        centralWidget->setLayout(layout);
        setCentralWidget(centralWidget);
    }

private slots:
    void fetchMarkets() {
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkRequest request(QUrl("http://localhost:8000/markets"));
        QNetworkReply *reply = manager->get(request);

        connect(reply, &QNetworkReply::finished, this, &MarketWindow::onMarketsFetched);
    }

    void onMarketsFetched() {
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (reply->error() == QNetworkReply::NoError) {
            QString response = reply->readAll();
            qDebug() << "Markets fetched: " << response;
        } else {
            qDebug() << "Error fetching markets: " << reply->errorString();
        }
        reply->deleteLater();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MarketWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"